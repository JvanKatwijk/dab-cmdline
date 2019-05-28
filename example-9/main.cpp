#
/*
 *    Copyright (C) 2015, 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB-library
 *
 *    DAB-library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB-library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB-library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	E X A M P L E  P R O G R A M
 *	This program might (or might not) be used to mould the interface to
 *	your wishes. Do not take it as a definitive and "ready" program
 *	for the DAB-library
 */
#include	<unistd.h>
#include	<signal.h>
#include	<getopt.h>
#include        <cstdio>
#include        <iostream>
#include	<complex>
#include	<vector>
#include	"audiosink.h"
#include	"filesink.h"
#include	"dab-api.h"
#include	"includes/support/band-handler.h"
#ifdef	HAVE_SDRPLAY
#include	"sdrplay-handler.h"
#elif	HAVE_AIRSPY
#include	"airspy-handler.h"
#elif	HAVE_RTLSDR
#include	"rtlsdr-handler.h"
#elif	HAVE_WAVFILES
#include	"wavfiles.h"
#elif	HAVE_RAWFILES
#include	"rawfiles.h"
#elif	HAVE_RTL_TCP
#include	"rtl_tcp-client.h"
#elif	HAVE_HACKRF
#include	"hackrf-handler.h"
#endif

#include	"dab-streamer.h"
#include	<locale>
#include	<codecvt>
#include	<atomic>
#include	<string>
using std::cerr;
using std::endl;

void    printOptions (void);	// forward declaration
//	we deal with callbacks from different threads. So, if you extend
//	the functions, take care and add locking whenever needed
static
std::atomic<bool> run;

static
void	*theRadio	= NULL;

static
audioBase	*soundOut	= NULL;

static
std::atomic<bool>timeSynced;

static
std::atomic<bool>timesyncSet;

static
std::atomic<bool>ensembleRecognized;

std::string	programName		= "Sky Radio";
int32_t		serviceIdentifier	= -1;

static void sighandler (int signum) {
        fprintf (stderr, "Signal caught, terminating!\n");
	run. store (false);
}

static
void	syncsignalHandler (bool b, void *userData) {
	timeSynced. store (b);
	timesyncSet. store (true);
	(void)userData;
}
//
//	This function is called whenever the dab engine has taken
//	some time to gather information from the FIC bloks
//	the Boolean b tells whether or not an ensemble has been
//	recognized, the names of the programs are in the 
//	ensemble
static
void	ensemblenameHandler (std::string name, int Id, void *userData) {
	fprintf (stderr, "ensemble %s is (%X) recognized\n",
	                          name. c_str (), (uint32_t)Id);
	ensembleRecognized. store (true);
}


std::vector<std::string> programNames;
std::vector<int> programSIds;

#include	<bits/stdc++.h>

std::unordered_map <int, std::string> ensembleContents;
static
void	programnameHandler (std::string s, int SId, void *userdata) {
	for (std::vector<std::string>::iterator it = programNames.begin();
	             it != programNames. end(); ++it)
	   if (*it == s)
	      return;
	ensembleContents. insert (pair <int, std::string> (SId, s));
	programNames. push_back (s);
	programSIds . push_back (SId);
	std::cerr << "program " << s << " is part of the ensemble\n";
}

static
void	programdataHandler (audiodata *d, void *ctx) {
	(void)ctx;
	std::cerr << "\tstartaddress\t= " << d -> startAddr << "\n";
	std::cerr << "\tlength\t\t= "     << d -> length << "\n";
	std::cerr << "\tsubChId\t\t= "    << d -> subchId << "\n";
	std::cerr << "\tprotection\t= "   << d -> protLevel << "\n";
	std::cerr << "\tbitrate\t\t= "    << d -> bitRate << "\n";
}

//
//	The function is called from within the library with
//	a string, the so-called dynamic label
static
void	dataOut_Handler (std::string dynamicLabel, void *ctx) {
	(void)ctx;
	soundOut	-> addRds (dynamicLabel. c_str ());
}
//
//	The function is called from the MOT handler, with
//	as parameters the filename where the picture is stored
//	d denotes the subtype of the picture 
//	typedef void (*motdata_t)(std::string, int, void *);
void	motdataHandler (std::string s, int d, void *ctx) {
	(void)s; (void)d; (void)ctx;
	fprintf (stderr, "plaatje %s\n", s. c_str ());
}

//
//	Note: the function is called from the tdcHandler with a
//	frame, either frame 0 or frame 1.
//	The frames are packed bytes, here an additional header
//	is added, a header of 8 bytes:
//	the first 4 bytes for a pattern 0xFF 0x00 0xFF 0x00 0xFF
//	the length of the contents, i.e. framelength without header
//	is stored in bytes 5 (high byte) and byte 6.
//	byte 7 contains 0x00, byte 8 contains 0x00 for frametype 0
//	and 0xFF for frametype 1
//	Note that the callback function is executed in the thread
//	that executes the tdcHandler code.
static
void	bytesOut_Handler (uint8_t *data, int16_t amount,
	                  uint8_t type, void *ctx) {
	(void)data;
	(void)amount;
	(void)ctx;
}
//
static
void	pcmHandler (int16_t *buffer, int size, int rate,
	                              bool isStereo, void *ctx) {
	   soundOut	->  audioOut (buffer, size, rate);
}

static
void	systemData (bool flag, int16_t snr, int32_t freqOff, void *ctx) {
//	fprintf (stderr, "synced = %s, snr = %d, offset = %d\n",
//	                    flag? "on":"off", snr, freqOff);
}

static
void	fibQuality	(int16_t q, void *ctx) {
//	fprintf (stderr, "fic quality = %d\n", q);
}

static
void	mscQuality	(int16_t fe, int16_t rsE, int16_t aacE, void *ctx) {
//	fprintf (stderr, "msc quality = %d %d %d\n", fe, rsE, aacE);
}

int	main (int argc, char **argv) {
// Default values
uint8_t		theMode		= 1;
int16_t		timeSyncTime	= 5;
int16_t		freqSyncTime	= 5;
std::string	fmFile		= std::string ("");
int		port		= -1;
std::string	url		= "127.0.0.1";
#ifdef	HAVE_WAVFILES
std::string	inputfileName	= std::string ("");
bool		repeater	= true;
#elif	HAVE_RAWFILES
std::string	inputfileName	= std::string ("");
bool		repeater	= true;
#elif	HAVE_HACKRF
uint8_t		theBand		= BAND_III;
std::string	theChannel	= "11C";
int		vgaGain		= 40;
int		lnaGain		= 40;
bool		ampEnable	= false;
int		ppmCorrection	= 0;
#elif	HAVE_SDRPLAY
uint8_t		theBand		= BAND_III;
std::string	theChannel	= "11C";
int		GRdB		= 20;
int		lnaState	= 2;
bool		autogain	= false;
int		ppmCorrection	= 0;
#elif	HAVE_RTLSDR
uint8_t		theBand		= BAND_III;
std::string	theChannel	= "11C";
int		gain		= 50;	// scale 1 .. 100
bool		autogain	= false;
int		ppmCorrection	= 0;
#elif	HAVE_AIRSPY
uint8_t		theBand		= BAND_III;
std::string	theChannel	= "11C";
int		gain		= 18;	// scale 1 .. 21
bool		boost		= false;
int		ppmCorrection	= 0;
#elif	HAVE_RTL_TCP
std::string	hostname = "127.0.0.1";		// default
int32_t		basePort = 1234;		// default
uint8_t		theBand		= BAND_III;
std::string	theChannel	= "11C";
int		gain		= 50;
bool		autogain	= false;
int		ppmCorrection;
#endif
std::string	soundChannel	= "default";
int		latency		= 10;
int		opt;
struct sigaction sigact;
bandHandler	dabBand;
deviceHandler	*theDevice;
int		frequency	= -1;
	std::cerr << "dab_cmdline example IX,\n \
	                Copyright 2017 J van Katwijk, Lazy Chair Computing\n";
	timeSynced.	store (false);
	timesyncSet.	store (false);
	run.		store (false);
	std::wcout.imbue(std::locale("en_US.utf8"));
	if (argc == 1) {
	   printOptions ();
	   exit (1);
	}

	std::setlocale (LC_ALL, "en-US.utf8");

//
//	Since different input devices have different approaches to
//	setting gain etc, we split

#ifdef HAVE_HACKRF
std::string options	= "M:D:d:P:S:p:u:f:C:B:v:l:ac:";
#elif	HAVE_SDRPLAY
std::string options	= "M:D:d:P:S:p:u:f:C:B:G:L:Qc:";
#elif	HAVE_RTLSDR
std::string options	= "M:D:d:P:S:p:u:f:C:B:G:Qc";
#elif	HAVE_AIRSPY
std::string options	= "M:D:d:P:S:p:u:f:C:B:G:bc:";
#elif   HAVE_RTL_TCP
std::string options	= "M:D:d:P:S:p:u:f:C:B:H:I:G:Qc:";
#elif	HAVE_WAVFILES
std::string options	= "M:D:d:P:S:p:u:f:M:F:R";
#elif	HAVE_RAWFILES
std::string options	= "M:D:d:P:S:p:u:f:M:F:R";
#endif
	while ((opt = getopt (argc, argv, options. c_str ())) != -1) {
	   switch (opt) {
//	first part is common
	      case 'M':
	         theMode	= atoi (optarg);
	         if (!((theMode == 1) || (theMode == 2) || (theMode == 4)))
	            theMode = 1; 
	         break;

	      case 'D':
	         freqSyncTime	= atoi (optarg);
	         break;

	      case 'd':
	         timeSyncTime	= atoi (optarg);
	         break;

	      case 'P':
	         programName	= std::string (optarg);
	         break;

	      case 'S': {
                 std::stringstream ss;
                 ss << std::hex << optarg;
//	ss >> serviceIdentifier;
                 break;
              }

	      case 'p':
	         port		= atoi (optarg);
	         break;

	      case 'u':
	         url		= std::string (optarg);
	         break;

	      case 'f':
	         fmFile		= std::string (optarg);
	         break;

#ifdef	HAVE_WAVFILES
	      case 'F':
	         inputfileName	= std::string (optarg);
	         break;

	      case 'R':
	         repeater	= false;
	         break;

#elif	HAVE_RAWFILES
	      case 'F':
	         inputfileName	= std::string (optarg);
	         break;
	      case 'R':
	         repeater	= false;
	         break;
#elif	HAVE_HACKRF
	      case 'B':
	         theBand = std::string (optarg) == std::string ("L_BAND") ?
	                                                 L_BAND : BAND_III;
	         break;

	      case 'C':
	         theChannel	= std::string (optarg);
	         break;

	      case 'v':
	         vgaGain	= atoi (optarg);
	         break;

	      case 'l':
	         lnaGain	= atoi (optarg);
	         break;

	      case 'a':
	         ampEnable	= true;
	         break;

	      case 'c':
	         ppmCorrection	= atoi (optarg);
	         break;

#elif	HAVE_SDRPLAY
	      case 'B':
	         theBand = std::string (optarg) == std::string ("L_BAND") ?
	                                                 L_BAND : BAND_III;
	         break;

	      case 'C':
	         theChannel	= std::string (optarg);
	         break;

	      case 'G':
	         GRdB		= atoi (optarg);
	         break;

	      case 'L':
	         lnaState	= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;

	      case 'c':
	         ppmCorrection	= atoi (optarg);
	         break;

#elif 	HAVE_RTLSDR
	      case 'B':
	         theBand = std::string (optarg) == std::string ("L_BAND") ?
	                                                 L_BAND : BAND_III;
	         break;

	      case 'C':
	         theChannel	= std::string (optarg);
	         break;

	      case 'G':
	         gain		= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;

	      case 'c':
	         ppmCorrection	= atoi (optarg);
	         break;

#elif	HAVE_AIRSPY
	      case 'B':
	         theBand = std::string (optarg) == std::string ("L_BAND") ?
	                                                 L_BAND : BAND_III;
	         break;

	      case 'C':
	         theChannel	= std::string (optarg);
	         break;

	      case 'G':
	         gain		= atoi (optarg);
	         break;
	
	      case 'b':
	         boost		= true;
	         break;

	      case 'c':
	         ppmCorrection	= atoi (optarg);
	         break;

#elif	HAVE_RTL_TCP
	      case 'B':
	         theBand = std::string (optarg) == std::string ("L_BAND") ?
	                                                 L_BAND : BAND_III;
	         break;

	      case 'C':
	         theChannel	= std::string (optarg);
	         break;

	      case 'H':
	         hostname	= std::string (optarg);
	         break;

	      case 'I':
	         basePort	= atoi (optarg);
	         break;

	      case 'G':
	         gain		= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;

	      case 'c':
	         ppmCorrection	= atoi (optarg);
	         break;
#endif

	      default:
	         printOptions ();
	         exit (1);
	   }
	}
//
	sigact.sa_handler = sighandler;
	sigemptyset (&sigact.sa_mask);
	sigact.sa_flags = 0;

	try {
#ifdef	HAVE_SDRPLAY
	   frequency	= dabBand. Frequency (theBand, theChannel);
	   theDevice	= new sdrplayHandler (frequency,
	                                      ppmCorrection,
	                                      GRdB,
	                                      lnaState,
	                                      autogain,
	                                      0,
	                                      0);
#elif	HAVE_AIRSPY
	   frequency	= dabBand. Frequency (theBand, theChannel);
	   theDevice	= new airspyHandler (frequency,
	                                     ppmCorrection,
	                                     gain,
	                                     boost);
#elif	HAVE_RTLSDR
	   frequency	= dabBand. Frequency (theBand, theChannel);
	   theDevice	= new rtlsdrHandler (frequency,
	                                     ppmCorrection,
	                                     gain,
	                                     autogain);
#elif	HAVE_HACKRF
	   frequency	= dabBand. Frequency (theBand, theChannel);
	   theDevice	= new hackrfHandler (frequency,
	                                      ppmCorrection,
	                                      lnaGain,
	                                      vgaGain,
	                                      ampEnable);
#elif	HAVE_RTL_TCP
	   frequency	= dabBand. Frequency (theBand, theChannel);
	   theDevice	= new rtl_tcp_client (hostname,
	                                      basePort,
	                                      frequency,
	                                      gain,
	                                      autogain,
	                                      ppmCorrection);
#elif	HAVE_WAVFILES
	   theDevice	= new wavFiles (inputfileName, repeater);
#elif	defined (HAVE_RAWFILES)
	   theDevice	= new rawFiles (inputfileName, repeater);
#endif

	}
	catch (int e) {
	   std::cerr << "allocating device failed (" << e << "), fatal\n";
	   exit (32);
	}
//
	if (theDevice == NULL) {
	   fprintf (stderr, "we do not seem to have a device\n");
	   exit (2);
	}
//	and with a sound device we now can create a "backend"
	theRadio	= dabInit (theDevice,
	                           theMode,
	                           syncsignalHandler,
	                           systemData,
	                           ensemblenameHandler,
	                           programnameHandler,
	                           fibQuality,
	                           pcmHandler,
	                           dataOut_Handler,
	                           bytesOut_Handler,
	                           programdataHandler,
	                           mscQuality,
	                           motdataHandler,	// MOT in PAD
	                           NULL,		// no spectrum shown
	                           NULL,		// no constellations
	                           NULL		// Ctx
	                          );
	if (theRadio == NULL) {
	   std::cerr << "sorry, no radio available, fatal\n";
	   exit (4);
	}

	theDevice	-> restartReader (frequency);
	if ((port == -1) && (fmFile. empty ())) {
	   bool err	= false;
	   soundOut	= new audioSink (latency, soundChannel, &err);
	   if (err) {
              std::cerr << "no valid sound channel, fatal\n";
              exit (33);
	   }
	}	
	else
	   try {
	      soundOut	= new dabStreamer (48000, fmFile, port, url);
	   } catch (int e) {
	      fprintf (stderr, "could not connect to fm server\n");
	      exit (34);
	   }
//
//	The device should be working right now

	timesyncSet.		store (false);
	ensembleRecognized.	store (false);
	dabStartProcessing (theRadio);

	while (!timeSynced. load () && (--timeSyncTime >= 0))
           sleep (1);

        if (!timeSynced. load ()) {
           cerr << "There does not seem to be a DAB signal here" << endl;
	   theDevice -> stopReader ();
           sleep (1);
           dabStop (theRadio);
           dabExit (theRadio);
           delete theDevice;
           exit (22);
	}
        else
	   std::cerr << "there might be a DAB signal here" << endl;

	while (!ensembleRecognized. load () &&
	                             (--freqSyncTime >= 0)) {
	   std::cerr << freqSyncTime + 1 << "\r";
	   sleep (1);
	}
	std::cerr << "\n";

	if (!ensembleRecognized. load ()) {
	   std::cerr << "no ensemble data found, fatal\n";
	   theDevice -> stopReader ();
	   sleep (1);
	   dabReset (theRadio);
	   dabExit  (theRadio);
	   delete theDevice;
	   exit (22);
	}

	run. store (true);
	std::cerr << "we try to start program " <<
                                                 programName << "\n";
	audiodata ad;
	if (!is_audioService (theRadio, programName. c_str ())) {
	   std::cerr << "sorry  we cannot handle service " << 
	                                         programName << "\n";
	   run. store (false);
	}

	if (run. load ()) {
	   dataforAudioService (theRadio,
	                     programName. c_str (), &ad, 0);
	   if (!ad. defined) {
	      std::cerr << "sorry  we cannot handle service " << 
	                                         programName << "\n";
	      run. store (false);
	   }
	}

	if (run. load ()) {
	   dabReset_msc (theRadio);
	   set_audioChannel (theRadio, &ad);
	}

	while (run. load ())
	   sleep (1);
	theDevice	-> stopReader ();
	dabReset (theRadio);
	dabExit  (theRadio);
	delete theDevice;	
	delete soundOut;
}

void    printOptions (void) {
        std::cerr << 
"                          dab-cmdline options are\n"
"	                  -M Mode\tMode is 1, 2 or 4. Default is Mode 1\n"
"	                  -D number\tamount of time to look for an ensemble\n"
"	                  -d number\tseconds to reach time sync\n"
"	                  -P name\tprogram to be selected in the ensemble\n"
"	                  -S hexnumber use hexnumber to identify program\n"
"	                  -f filename\twrite audio as stereo fm data to file\n"
"	                  -p port\twrite audio as stereo fm to port port\n"
"	                  -u url\twrite audio as stereo fm to given port on url\n"
"	for file input:\n"
"	                  -F filename\tin case the input is from file\n"
"	                  -R switch off automatic continuation after eof\n"
"	for hackrf:\n"
"	                  -B Band\tBand is either L_BAND or BAND_III (default)\n"
"	                  -C Channel\n"
"	                  -v vgaGain\n"
"	                  -l lnaGain\n"
"	                  -a amp enable (default off)\n"
"	                  -c number\tppm offset\n"
"	for SDRplay:\n"
"	                  -B Band\tBand is either L_BAND or BAND_III (default)\n"
"	                  -C Channel\n"
"	                  -G Gain reduction in dB (range 20 .. 59)\n"
"	                  -L lnaState (depends on model chosen)\n"
"	                  -Q autogain (default off)\n"
"	                  -c number\t ppm offset\n"
"	for rtlsdr:\n"
"	                  -B Band\tBand is either L_BAND or BAND_III (default)\n"
"	                  -C Channel\n"
"	                  -G number\t	gain, range 0 .. 100\n"
"	                  -Q autogain (default off)\n"
"	                  -c number\tppm offset\n"
"	for airspy:\n"
"	                  -B Band\tBand is either L_BAND or BAND_III (default)\n"
"	                  -C Channel\n"
"	                  -G number\t	gain, range 1 .. 21\n"
"	                  -b set rf bias\n"
"	                  -c number\t ppm Correction\n"
"	for rtl_tcp:\n"
"	                  -H url\t hostname for connection\n"
"	                  -I number\t baseport for connection\n"
"	                  -G number\t gain setting\n"
"	                  -Q autogain (default off)\n"
"	                  -c number\t ppm Correction\n";
}

