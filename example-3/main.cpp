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
#include	<complex>
#include	<vector>
#include	"dab-api.h"
#include	"includes/support/band-handler.h"
#ifdef  HAVE_SDRPLAY
#include        "sdrplay-handler.h"
#elif   HAVE_AIRSPY
#include        "airspy-handler.h"
#elif   HAVE_RTLSDR
#include        "rtlsdr-handler.h"
#elif   HAVE_WAVFILES
#include        "wavfiles.h"
#elif   HAVE_RAWFILES
#include        "rawfiles.h"
#elif   HAVE_RTL_TCP
#include        "rtl_tcp-client.h"
#elif   HAVE_HACKRF
#include        "hackrf-handler.h"
#elif   HAVE_LIME
#include        "lime-handler.h"
#endif

#include        <locale>
#include        <codecvt>
#include	<atomic>
#ifdef	DATA_STREAMER
#include	"tcp-server.h"
#endif

#ifdef	STREAMER_OUTPUT
#include	"streamer.h"
#endif
using std::cerr;
using std::endl;

void    printOptions (void);	// forward declaration
//	we deal with some callbacks, so we have some data that needs
//	to be accessed from global contexts
static
std::atomic<bool> run;

static
void	*theRadio	= NULL;

#ifdef	STREAMER_OUTPUT
static
streamer	*theStreamer	= NULL;
#endif

static
std::atomic<bool>timeSynced;

static
std::atomic<bool>timesyncSet;

static
std::atomic<bool>ensembleRecognized;

#ifdef	DATA_STREAMER
tcpServer	tdcServer (8888);
#endif

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

static
void	programnameHandler (std::string s, int SId, void * userdata) {
	fprintf (stderr, "%s (%X) is part of the ensemble\n", s. c_str (), SId);
}

static
void	programdataHandler (audiodata *d, void *ctx) {
	(void)ctx;
	fprintf (stderr, "\tstartaddress\t= %d\n", d -> startAddr);
	fprintf (stderr, "\tlength\t\t= %d\n",     d -> length);
	fprintf (stderr, "\tsubChId\t\t= %d\n",    d -> subchId);
	fprintf (stderr, "\tprotection\t= %d\n",   d -> protLevel);
	fprintf (stderr, "\tbitrate\t\t= %d\n",    d -> bitRate);
}

//
//	The function is called from within the library with
//	a string, the so-called dynamic label
static
void	dataOut_Handler (std::string dynamicLabel, void *ctx) {
	(void)ctx;
	fprintf (stderr, "%s\r", dynamicLabel. c_str ());
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
//
//	In this example the PCM samples are written out to stdout.
//	In order to fill "gaps" in the output, the output is send
//	through a simple task, monitoring the amount of samples
//	passing by and sending additional 0 samples in case
//	of gaps
//
static
void	pcmHandler (int16_t *buffer, int size, int rate,
	                              bool isStereo, void *ctx) {
#ifdef	STREAMER_OUTPUT
	if (theStreamer == NULL)
	   return;
	if (!theStreamer -> isRunning ())
	   theStreamer -> restart ();
	theStreamer -> addBuffer (buffer, size, 2);
#else
	fwrite ((void *)buffer, size, 2, stdout);
#endif
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
#ifdef	HAVE_HACKRF
int		lnaGain		= 40;
int		vgaGain		= 40;
int		ppmOffset	= 0;
const char	*optionsString	= "D:d:M:B:P:O:A:C:G:g:p:";
#elif	HAVE_LIME
int16_t		gain		= 70;
std::string	antenna		= "Auto";
const char	*optionsString	= "D:d:M:B:P:O:A:C:G:g:X:";
#elif	HAVE_SDRPLAY	
int16_t		GRdB		= 30;
int16_t		lnaState	= 2;
bool		autogain	= false;
int16_t		ppmOffset	= 0;
const char	*optionsString	= "D:d:M:B:P:O:A:C:G:L:Qp:";
#elif	HAVE_AIRSPY
int16_t		gain		= 20;
bool		autogain	= false;
int		ppmOffset	= 0;
const char	*optionsString	= "D:d:M:B:P:O:A:C:G:p:";
#elif	HAVE_RTLSDR
int16_t		gain		= 50;
bool		autogain	= false;
int16_t		ppmOffset	= 0;
const char	*optionsString	= "D:d:M:B:P:O:A:C:G:p:Q";
#elif	HAVE_WAVFILES
std::string	fileName;
bool		repeater	= true;
const char	*optionsString	= "D:d:M:B:P:O:A:F:R:";
#elif	HAVE_RAWFILES
std::string	fileName;
bool	repeater		= true;
const char	*optionsString	= "D:d:M:B:P:O:A:F:R:";
#elif
//	HAVE_RTL_TCP
int		gain		= 50;
bool		autogain	= false;
int		ppmOffset	= 0;
std::string	hostname = "127.0.0.1";		// default
int32_t		basePort = 1234;		// default
const char	*optionsString	= "D:d:M:B:P:O:A:C:G:Qp:H:I";
#endif
std::string	soundChannel	= "default";
int16_t		latency		= 10;
int16_t		timeSyncTime	= 5;
int16_t		freqSyncTime	= 5;
int		opt;
struct sigaction sigact;
bandHandler	dabBand;
deviceHandler	*theDevice;
bool	err;

	fprintf (stderr, "dab_cmdline example III,\n \
	                  Copyright 2017 J van Katwijk, Lazy Chair Computing\n");
	timeSynced.	store (false);
	timesyncSet.	store (false);
	run.		store (false);

	std::setlocale (LC_ALL, "");
	if (argc == 1) {
	   printOptions ();
	   exit (1);
	}

	fprintf (stderr, "options are %s\n", optionsString);
	while ((opt = getopt (argc, argv, optionsString)) != -1) {
	   switch (opt) {
	      case 'D':
	         freqSyncTime	= atoi (optarg);
	         break;

	      case 'd':
	         timeSyncTime	= atoi (optarg);
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

#ifdef	HAVE_WAVFILES
	      case 'F':
	         fileName	= std::string (optarg);

	      case 'R':
	         repeater	= false;
	         break;
#elif	HAVE_RAWFILES
	      case 'F':
	         fileName	= std::string (optarg);
	         break;

	      case 'R':	         repeater	= false;
	         break;

#elif	HAVE_HACKRF
	      case 'G':
	         lnaGain	= atoi (optarg);
	         break;

	      case 'g':
	         vgaGain	= atoi (optarg);
	         break;

	      case 'C':
	         theChannel	= std::string (optarg);
	         break;
	
	      case 'p':
	         ppmOffset	= 0;
	         break;

#elif	HAVE_LIME
	      case 'G':
	      case 'g':	
	         gain		= atoi (optarg);
	         break;

	      case 'X':
	         antenna	= std::string (optarg);
	         break;

	      case 'C':
	         theChannel	= std::string (optarg);
	         fprintf (stderr, "%s \n", optarg);
	         break;

#elif	HAVE_SDRPLAY
	      case 'G':
	         GRdB		= atoi (optarg);
	         break;

	      case 'L':
	         lnaState	= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;

	      case 'C':
	         theChannel	= std::string (optarg);
	         break;

	      case 'p':
	         ppmOffset	= atoi (optarg);
	         break;

#elif	HAVE_AIRSPY
	      case 'G':
	         gain		= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;

	      case 'p':
	         ppmOffset	= atoi (optarg);
	         break;

	      case 'C':
	         theChannel	= std::string (optarg);
	         break;

#elif	HAVE_RTLSDR
	      case 'G':
	         gain		= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;

	      case 'p':
	         ppmOffset	= atoi (optarg);
	         break;

	      case 'C':
	         theChannel	= std::string (optarg);
	         break;

#elif	HAVE_RTL_TCP
	      case 'C':
	         theChannel	= std::string (optarg);
	         break;

	      case 'H':
	         hostname	= std::string (optarg);
	         break;

	      case 'I':
	         basePort	= atoi (optarg);
	         break;
	      case "G":
	         gain		= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;

	      case 'p':
	         ppmOffset	= atoi (optarg);
	         break;

#endif

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
	                                      ppmOffset,
	                                      GRdB,
	                                      lnaState,
	                                      autogain,
	                                      0,
	                                      0);
#elif	HAVE_AIRSPY
	   theDevice	= new airspyHandler (frequency,
	                                     ppmOffset,
	                                     gain, false);
#elif	HAVE_RTLSDR
	   theDevice	= new rtlsdrHandler (frequency,
	                                     ppmOffset,
	                                     gain,
	                                     autogain);
#elif   HAVE_HACKRF
           theDevice    = new hackrfHandler     (frequency,
                                                 ppmCorrection,
                                                 lnaGain,
                                                 vgaGain);
#elif   HAVE_LIME
           theDevice    = new limeHandler       (frequency, gain, antenna);
#elif	HAVE_WAVFILES
	   theDevice	= new wavFiles (fileName, repeater);
#elif	HAVE_RAWFILES
	   theDevice	= new rawFiles (fileName, repeater);
#elif	HAVE_RTL_TCP
	   theDevice	= new rtl_tcp_client (hostname,
	                                      basePort,
	                                      frequency,
	                                      gain,
	                                      autogain,
	                                      ppmOffset);
#endif
	}
	catch (int e) {
	   fprintf (stderr, "allocating device failed (%d), fatal\n", e);
	   exit (32);
	}
#ifdef	STREAMER_OUTPUT
	theStreamer	= new streamer ();
#endif
//	and with a sound device we can create a "backend"
	theRadio	= (void *)dabInit (theDevice,
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
	                                   NULL,	// no mot slides
	                                   NULL,	// no spectrum shown
	                                   NULL,	// no constellations
	                                   NULL		//ctx
	                               );
	if (theRadio == NULL) {
	   fprintf (stderr, "sorry, no radio available, fatal\n");
	   exit (4);
	}

	theDevice	-> restartReader (frequency);
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
           dabStop	(theRadio);
	   dabExit	(theRadio);
           delete theDevice;
           exit (22);
	}
        else
	   cerr << "there might be a DAB signal here" << endl;

	if (!ensembleRecognized. load ())
	while (!ensembleRecognized. load () &&
                                     (--freqSyncTime >= 0)) {
           std::cerr << freqSyncTime + 1 << "\r";
           sleep (1);
        }
        std::cerr << "\n";

	if (!ensembleRecognized. load ()) {
	   fprintf (stderr, "no ensemble data found, fatal\n");
	   theDevice -> stopReader ();
	   sleep (1);
	   dabStop	(theRadio);
	   dabExit	(theRadio);
	   delete theDevice;
	   exit (22);
	}

	run. store (true);
	if (serviceIdentifier != -1) 
	   programName = dab_getserviceName (theRadio, serviceIdentifier);

	std::cerr << "we try to start program " <<
                                                 programName << "\n";
	if (!is_audioService (theRadio, programName. c_str ())) {
	   std::cerr << "sorry  we cannot handle service " <<
                                                 programName << "\n";
	   run. store (false);
	}

	audiodata ad;
	dataforAudioService (theRadio, programName. c_str (), &ad, 0);
	if (!ad. defined) {
	   std::cerr << "sorry  we cannot handle service " <<
                                                 programName << "\n";
	   run. store (false);
	}

	dabReset_msc (theRadio);
	set_audioChannel (theRadio, &ad);

	while (run. load ())
	   sleep (1);
	theDevice	-> stopReader ();
	dabStop (theRadio);
	dabExit	(theRadio);
	delete theDevice;	
}

void    printOptions (void) {
        fprintf (stderr,
"                          dab-cmdline options are\n\
                          -W number   amount of time to look for an ensemble\n\
                          -M Mode     Mode is 1, 2 or 4. Default is Mode 1\n\
                          -B Band     Band is either L_BAND or BAND_III (default)\n\
                          -P name     program to be selected in the ensemble\n\
                          -C channel  channel to be used\n\
                          -G Gain     gain for device (range 1 .. 100)\n\
                          -Q          if set, set autogain for device true\n\
	                  -F filename in case the input is from file\n\
                          -S hexnumber use hexnumber to identify program\n\n");
}

                          

