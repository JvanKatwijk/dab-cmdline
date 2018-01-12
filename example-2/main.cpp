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
 *	for the DAB-library
 */
#include	<unistd.h>
#include	<signal.h>
#include	<getopt.h>
#include        <cstdio>
#include        <iostream>
#include	"audiosink.h"
#include	"filesink.h"
#include	"dab-class.h"
#include	"band-handler.h"
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
#endif
#include	<locale>
#include	<codecvt>
#include	<atomic>
#ifdef	DATA_STREAMER
#include	"tcp-server.h"
#endif
#include	<string>
using std::cerr;
using std::endl;

void    printOptions (void);	// forward declaration
//	we deal with some callbacks, so we have some data that needs
//	to be accessed from global contexts
static
std::atomic<bool> run;

static
dabClass	*theRadio	= NULL;

static
std::atomic<bool>timeSynced;

static
std::atomic<bool>timesyncSet;

static
std::atomic<bool>ensembleRecognized;

static
audioBase	*soundOut	= NULL;

#ifdef	DATA_STREAMER
tcpServer	tdcServer (8888);
#endif

std::string	programName		= "Classic FM";
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

static
void	programnameHandler (std::string s, int SId, void *userdata) {
	for (std::vector<std::string>::iterator it = programNames.begin();
	             it != programNames. end(); ++it)
	   if (*it == s)
	      return;
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
	std::cerr << dynamicLabel << "\r";
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
#ifdef DATA_STREAMER
uint8_t localBuf [amount + 8];
int16_t i;
	localBuf [0] = 0xFF;
	localBuf [1] = 0x00;
	localBuf [2] = 0xFF;
	localBuf [3] = 0x00;
	localBuf [4] = (amount >> 8) & 0xFF;
	localBuf [5] = amount & 0xFF;
	localBuf [6] = 0x00;
	localBuf [7] = type == 0 ? 0 : 0xFF;
	for (i = 0; i < amount; i ++)
	   localBuf [8 + i] = data;
	tdcServer. sendData (localBuf, amount + 8);
#else
	(void)data;
	(void)amount;
#endif
	(void)ctx;
}
//
//	This function is overloaded. In the normal form it
//	handles a buffer full of PCM samples. We pass them on to the
//	audiohandler, based on portaudio. Feel free to modify this
//	and send the samples elsewhere
//
//	However, in the "special mode", the aac frames are send out
//	Obviously, the parameters "rate" and "isStereo" are meaningless
//	then.
static
void	pcmHandler (int16_t *buffer, int size, int rate,
	                              bool isStereo, void *ctx) {
static bool isStarted	= false;

	(void)isStereo;
	if (!isStarted) {
	   soundOut	-> restart ();
	   isStarted	= true;
	}
	soundOut	-> audioOut (buffer, size, rate);
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
std::string	theChannel	= "11C";
uint8_t		theBand		= BAND_III;
int16_t		ppmCorrection	= 0;
int		theGain		= 45;	// scale = 0 .. 100
std::string	soundChannel	= "default";
int16_t		latency		= 10;
int16_t		waitingTime	= 10;
bool		autogain	= false;
int		opt;
struct sigaction sigact;
bandHandler	dabBand;
deviceHandler	*theDevice;
#ifdef	HAVE_WAVFILES
std::string	fileName;
#elif	HAVE_RAWFILES
std::string	fileName;
#elif HAVE_RTL_TCP
std::string	hostname = "127.0.0.1";		// default
int32_t		basePort = 1234;		// default
#endif
bool	err;

	std::cerr << "dab_cmdline V 1.0alfa,\n \
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

//	For file input we do not need options like Q, G and C,
//	We do need an option to specify the filename
#if	(!defined (HAVE_WAVFILES) && !defined (HAVE_RAWFILES))
	while ((opt = getopt (argc, argv, "W:M:B:C:P:G:A:L:S:QO:")) != -1) {
#elif   HAVE_RTL_TCP
	while ((opt = getopt (argc, argv, "W:M:B:C:P:G:A:L:S:H:I:QO:")) != -1) {
#else
	while ((opt = getopt (argc, argv, "W:M:B:P:A:L:S:F:O:")) != -1) {
#endif
	   switch (opt) {

	      case 'W':
	         waitingTime	= atoi (optarg);
	         break;

	      case 'M':
	         theMode	= atoi (optarg);
	         if (!((theMode == 1) || (theMode == 2) || (theMode == 4)))
	            theMode = 1; 
	         break;

	      case 'B':
	         theBand = std::string (optarg) == std::string ("L_BAND") ?
	                                                 L_BAND : BAND_III;
	         break;

	      case 'P':
	         programName	= optarg;
	         break;

	      case 'p':
	         ppmCorrection	= atoi (optarg);
	         break;

#ifdef	HAVE_WAVFILES
	      case 'F':
	         fileName	= std::string (optarg);
	         break;
#elif	HAVE_RAWFILES
	      case 'F':
	         fileName	= std::string (optarg);
	         break;
#else
	      case 'C':
	         theChannel	= std::string (optarg);
	         break;

	      case 'G':
	         theGain	= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;

#ifdef	HAVE_RTL_TCP
	      case 'H':
	         hostname	= std::string (optarg);
	         break;

	      case 'I':
	         basePort	= atoi (optarg);
	         break;
#endif
#endif

	      case 'O':
	         soundOut	= new fileSink (std::string (optarg), &err);
	         if (!err) {
	            std::cerr << "sorry, could not open file\n";
	            exit (32);
	         }
	         break;

	      case 'A':
	         soundChannel	= optarg;
	         break;

	      case 'L':
	         latency	= atoi (optarg);
	         break;

	      case 'S': {
                 std::stringstream ss;
                 ss << std::hex << optarg;
                 ss >> serviceIdentifier;
                 break;
              }

	      default:
	         printOptions ();
	         exit (1);
	   }
	}
//
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;

	int32_t frequency	= dabBand. Frequency (theBand, theChannel);
	try {
#ifdef	HAVE_SDRPLAY
	   theDevice	= new sdrplayHandler (frequency,
	                                      ppmCorrection,
	                                      theGain,
	                                      autogain,
	                                      0,
	                                      0);
#elif	HAVE_AIRSPY
	   theDevice	= new airspyHandler (frequency,
	                                     ppmCorrection,
	                                     theGain);
#elif	HAVE_RTLSDR
	   theDevice	= new rtlsdrHandler (frequency,
	                                     ppmCorrection,
	                                     theGain,
	                                     autogain);
#elif	HAVE_WAVFILES
	   theDevice	= new wavFiles (fileName);
#elif	defined (HAVE_RAWFILES)
	   theDevice	= new rawFiles (fileName);
#elif	HAVE_RTL_TCP
	   theDevice	= new rtl_tcp_client (hostname,
	                                      basePort,
	                                      frequency,
	                                      theGain,
	                                      autogain,
	                                      ppmCorrection);
#endif

	}
	catch (int e) {
	   std::cerr << "allocating device failed (" << e << "), fatal\n";
	   exit (32);
	}
//
	if (soundOut == NULL) {	// not bound to a file?
	   soundOut	= new audioSink	(latency, soundChannel, &err);
	   if (err) {
	      std::cerr << "no valid sound channel, fatal\n";
	      exit (33);
	   }
	}
//
//	and with a sound device we can create a "backend"
	theRadio	= new dabClass (theDevice,
	                                theMode,
	                                NULL,		// no spectrum shown
	                                NULL,		// no constellations
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
	                                NULL,
	                                NULL
	                               );
	if (theRadio == NULL) {
	   std::cerr << "sorry, no radio available, fatal\n";
	   exit (4);
	}

	theDevice	-> setGain (theGain);
	if (autogain)
	   theDevice	-> set_autogain (autogain);
	theDevice	-> setVFOFrequency (frequency);
	theDevice	-> restartReader ();
//
//	The device should be working right now

	timesyncSet.		store (false);
	ensembleRecognized.	store (false);
	theRadio -> startProcessing ();

	int	timeOut	= 0;
//	while (!timesyncSet. load () && (++timeOut < 5))
	while (++timeOut < waitingTime)
           sleep (1);

        if (!timeSynced. load ()) {
           cerr << "There does not seem to be a DAB signal here" << endl;
	   theDevice -> stopReader ();
           sleep (1);
           theRadio     -> stop ();
           delete theRadio;
           delete theDevice;
           exit (22);

	}
        else
	   std::cerr << "there might be a DAB signal here" << endl;

	if (!ensembleRecognized. load ())
	   while (!ensembleRecognized. load () && (++timeOut < waitingTime)) {
	      std::cerr << waitingTime - timeOut << "\r";
	      sleep (1);
	   }
	std::cerr << "\n";

	if (!ensembleRecognized. load ()) {
	   std::cerr << "no ensemble data found, fatal\n";
	   theDevice -> stopReader ();
	   sleep (1);
	   theRadio	-> reset ();
	   delete theRadio;
	   delete theDevice;
	   exit (22);
	}

	run. store (true);
	if (serviceIdentifier != -1) 
	   programName = theRadio -> dab_getserviceName (serviceIdentifier);
	std::cerr << "we try to start program " <<
                                                 programName << "\n";
	if (theRadio -> dab_service (programName) < 0) {
	   std::cerr << "sorry  we cannot handle service " << 
	                                         programName << "\n";
	   run. store (false);
	}

	while (run. load ())
	   sleep (1);
	theDevice	-> stopReader ();
	theRadio	-> reset ();
	delete theRadio;
	delete theDevice;	
	delete soundOut;
}

void    printOptions (void) {
        std::cerr << 
"                          dab-cmdline options are\n\
                          -W number   amount of time to look for an ensemble\n\
                          -M Mode     Mode is 1, 2 or 4. Default is Mode 1\n\
                          -B Band     Band is either L_BAND or BAND_III (default)\n\
                          -P name     program to be selected in the ensemble\n\
                          -C channel  channel to be used\n\
                          -G Gain     gain for device (range 1 .. 100)\n\
                          -Q          if set, set autogain for device true\n\
	                  -F filename in case the input is from file\n\
                          -A name     select the audio channel (portaudio)\n\
                          -L number   latency for audiobuffer\n\
                          -S hexnumber use hexnumber to identify program\n\n\
	                  -O filename put the output into a file rather than through portaudio\n";
}

