#
/*
 *    Copyright (C) 2015, 2016, 2025
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
#include	<stdlib.h>
#include	<unistd.h>
#include	<signal.h>
#include	<getopt.h>
#include        <cstdio>
#include	<vector>
#include	<iostream>
#include	"dab-api.h"
#include	"includes/support/band-handler.h"
#include	"tii-handler.h"
#include	"service-printer.h"
#ifdef	HAVE_SDRPLAY
#include	"sdrplay-handler.h"
#elif	HAVE_AIRSPY
#include	"airspy-handler.h"
#elif	HAVE_RTLSDR
#include	"rtlsdr-handler.h"
#elif	HAVE_HACKRF
#include	"hackrf-handler.h"
#elif	HAVE_LIME
#include	"lime-handler.h"
#elif	HAVE_SDRPLAY_V3
#include	"sdrplay-handler-v3.h"
#endif

#include	<atomic>
#include	<thread>

using std::cerr;
using std::endl;

void    printOptions	();	// forward declaration
//	we deal with some callbacks, so we have some data that needs
//	to be accessed from global contexts
static
std::atomic<bool> run;

static
void	*theRadio	= nullptr;

static
tiiHandler the_tiiHandler;

static
std::atomic<bool>timeSynced;

static
std::atomic<bool>timesyncSet;

static
std::atomic<bool>ensembleRecognized;

static
FILE	*outFile	= stdout;

static void sighandler (int signum) {
	fprintf (stderr, "Signal caught, terminating!\n");
	run. store (false);
}

static
void	syncsignalHandler (bool b, void *userData) {
	timeSynced.	store (b);
	timesyncSet.	store (true);
	(void)userData;
}
//
std::string	ensembleName;
uint32_t	ensembleId;
static
void	name_of_ensemble (const std::string &name, int Id, void *userData) {
	fprintf (stderr, "ensemble %s is (%X) recognized\n",
	                          name. c_str (), (uint32_t)Id);
	ensembleRecognized. store (true);
	ensembleName	= name;
	ensembleId	= Id;
}

std::vector<std::string> programNames;
std::vector<int> programSIds;

static
void	serviceName (const std::string &s, int SId,
	                                 uint16_t subChId, void *userdata) {
	for (std::vector<std::string>::iterator it = programNames.begin();
	             it != programNames. end(); ++it)
	   if (*it == s)
	      return;
	programNames. push_back (s);
	programSIds . push_back (SId);
//	fprintf (stderr, "program %s is part of the ensemble\n", s. c_str ());
}

static
void	tii_data_Handler	(tiiData *theData, void *ctx) {
	the_tiiHandler. add (*theData);
	(void)ctx;
}

static
void	programdata_Handler (audiodata *d, void *ctx) {
	(void) *d;
	(void)ctx;
}
//
//	The function is called from within the library with
//	a string, the so-called dynamic label
static
void	dataOut_Handler (const char *dynamicLabel, void *ctx) {
	(void)ctx;
//	fprintf (stderr, "%s\n", dynamicLabel);
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
	(void)amount; (void)type;
	(void)ctx;
}
//
static
void	pcmHandler (int16_t *buffer, int size, int rate,
	                              bool isStereo, void *ctx) {
	(void)buffer;
	(void)size;
	(void)rate;
	(void)ctx;
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
std::string	startChannel	= "5A";
uint8_t		theMode		= 1;
std::string	theChannel	= "5C";
uint8_t		theBand		= BAND_III;
#ifdef	HAVE_HACKRF
int		lnaGain		= 40;
int		vgaGain		= 40;
int		ppmOffset	= 0;
const char	*optionsString	= "I:F:jD:M:B:C::G:g:p:";
#elif	HAVE_LIME
int16_t		gain		= 70;
std::string	antenna		= "Auto";
const char	*optionsString	= "I:F:jD:M:B:C:G:g:X:";
#elif	HAVE_SDRPLAY	
int16_t		GRdB		= 30;
int16_t		lnaState	= 4;
bool		autogain	= true;
int16_t		ppmOffset	= 0;
const char	*optionsString	= "I:F:jD:M:B:C:G:L:Qp:";
#elif	HAVE_SDRPLAY_V3	
int16_t		GRdB		= 30;
int16_t		lnaState	= 2;
bool		autogain	= true;
int16_t		ppmOffset	= 0;
const char	*optionsString	= "I:F:jD:M:B:C:G:L:Qp:";
#elif	HAVE_AIRSPY
int16_t		gain		= 20;
bool		autogain	= false;
bool		rf_bias		= false;
const char	*optionsString	= "I:F:jD:M:B:C:G:bp:";
#elif	HAVE_RTLSDR
int16_t		gain		= 50;
bool		autogain	= false;
int16_t		ppmOffset	= 0;
int		dumpDuration	= 1;
bool		rawDump		= false;
const char	*optionsString	= "I:F:jD:M:B:C:G:p:QR:T:";
#endif
int	opt;
int	freqSyncTime		= 8;
int	tiiSyncTime		= 10;
bool	jsonOutput		= false;
struct sigaction sigact;
bandHandler	dabBand;
deviceHandler	*theDevice;
bool firstEnsemble = true;

	fprintf (stderr, "dab_scanner V 2.0alfa,\n"
	                "Copyright 2018 J van Katwijk, Lazy Chair Computing\n"	                         "2018 Hayati Ayguen\n"
	                 "2019 J van Katwijk\n"
	                 "2020 J van Katwijk\n"
	                 "2025 J van Katwijk\n");
	timeSynced.	store (false);
	timesyncSet.	store (false);
	run.		store (false);

	if (argc == 1) {
	   printOptions ();
	   exit (1);
	}

	while ((opt = getopt (argc, argv, optionsString)) != -1) {
	   switch (opt) {
	      case 'F':
	         outFile	= fopen (optarg, "w");
	         if (outFile == nullptr)
	            outFile = stderr;
	         break;

	      case 'j':
	         jsonOutput	= true;
	         break;

	      case 'D':
	         freqSyncTime	= atoi (optarg);
	         break;

	      case 'I':
	         tiiSyncTime	= atoi (optarg);
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

	      case 'C':
	         startChannel	= std::string (optarg);
	         break;


//	device specific options

#ifdef	HAVE_HACKRF
	      case 'G':
	         lnaGain	= atoi (optarg);
	         break;

	      case 'g':
	         vgaGain	= atoi (optarg);
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

	      case 'p':
	         ppmOffset	= atoi (optarg);
	         break;

#elif	HAVE_SDRPLAY_V3
	      case 'G':
	         GRdB		= atoi (optarg);
	         break;

	      case 'L':
	         lnaState	= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
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

	      case 'R':
	         rawDump	= true;
	         break;

	      case 'T':
	         dumpDuration	= atoi (optarg);
	         break;
#endif

	      default:
	         printOptions ();
	         exit (1);
	   }
	}
//
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;

	theChannel		= startChannel;
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
#elif   HAVE_SDRPLAY_V3
           theDevice    = new sdrplayHandler_v3 (frequency,
                                              ppmOffset,
                                              GRdB,
                                              lnaState,
                                              autogain,
                                              0,
                                              0);
#elif	HAVE_AIRSPY
	   theDevice	= new airspyHandler (frequency,
	                                     ppmOffset,
	                                     gain,
	                                     rf_bias);
#elif	HAVE_RTLSDR
	   theDevice	= new rtlsdrHandler (frequency,
	                                     ppmOffset,
	                                     gain,
	                                     autogain);
#elif	HAVE_HACKRF
	   theDevice	= new hackrfHandler (frequency,
	                                     ppmOffset,
	                                     lnaGain,
	                                     vgaGain);
#endif
	}
	catch (int e) {
	   fprintf (stderr, "allocating device failed (%d), fatal\n", e);
	   exit (32);
	}

//	and with a sound device we now can create a "backend"
        API_struct interface;
        interface. dabMode      = theMode;
	interface. thresholdValue	= 6;
        interface. syncsignal_Handler   = syncsignalHandler;
        interface. systemdata_Handler   = systemData;
        interface. name_of_ensemble 	= name_of_ensemble;
        interface. serviceName  	= serviceName;
        interface. fib_quality_Handler  = fibQuality;
        interface. audioOut_Handler     = pcmHandler;
        interface. dataOut_Handler      = dataOut_Handler;
        interface. bytesOut_Handler     = bytesOut_Handler;
        interface. programdata_Handler  = programdata_Handler;
        interface. program_quality_Handler              = mscQuality;
        interface. motdata_Handler	= nullptr;
        interface. tii_data_Handler	= tii_data_Handler;
        interface. timeHandler		= nullptr;
//
//	and with a sound device we can create a "backend"
	theRadio	= dabInit (theDevice,
	                           &interface,
	                           nullptr,		// no spectrum shown
	                           nullptr,		// no constellations
	                           nullptr
	                          );
	if (theRadio == nullptr) {
	   fprintf (stderr, "sorry, no radio device available, fatal\n");
	   exit (4);
	}

//	theDevice	-> setGain (theGain);
	if (autogain)
	   theDevice	-> set_autogain (autogain);

   	print_fileHeader (outFile, jsonOutput);
   
	while (true) {
	   bool	firstTime	= true;
	   bool firstService	= true;
	   int	the_timeSyncTime	= 5;
	   int	the_freqSyncTime	= freqSyncTime;
	   int	the_tiiSyncTime		= tiiSyncTime;

	   theDevice	-> stopReader ();
	   the_tiiHandler. stop ();
	   int32_t frequency =
	               dabBand. Frequency (theBand, theChannel);
	   theDevice	-> restartReader (frequency);
	   ensembleRecognized.	store (false);
	   dabReset (theRadio);
//	The device should be working right now

	   fprintf (stderr, "checking data in channel %s\n",
	                                          theChannel. c_str ());
	   timesyncSet.		store (false);
	   timeSynced. 		store (false);

	   while (!timesyncSet. load () && (--the_timeSyncTime >= 0)) {
	      fprintf (stderr, "Waiting for time sync %d\r", the_timeSyncTime);
	      sleep (1);
	   }

	   if (!timeSynced. load ()) {
	      theChannel = dabBand. nextChannel (theBand, theChannel);
	      if (theChannel == startChannel)
	         break;
	      else
	         continue;
	   }
//
//	we might have data here, not sure yet
	   while (!ensembleRecognized. load () &&
	                             (--the_freqSyncTime >= 0)) {
	       fprintf (stderr, "waiting for frequency sync %d\r", the_freqSyncTime);
	       sleep (1);
	   }
	   fprintf (stderr, "\n");

	   if (!ensembleRecognized. load ()) {
	      theChannel = dabBand. nextChannel (theBand, theChannel);
	      if (theChannel == startChannel)
	         break;
	      else
	         continue;
	   }

	   the_tiiHandler. start (ensembleId);
#ifdef	HAVE_RTLSDR
	   if (rawDump) {
	      ((rtlsdrHandler *)theDevice) -> startDumping (theChannel,
	                                                     ensembleId);
	      for (int i = 0; i < dumpDuration; i ++) {
	         sleep (1);
	         fprintf (stderr, "%d\r", dumpDuration - i);
	      }
	   }
#endif
	   while (--the_tiiSyncTime > 0) {
	      fprintf (stderr, "Looking for TII signals %d\r", the_tiiSyncTime);
	      sleep (1);
	   }
//	print ensemble data here
	   print_ensembleData (outFile,
	                       jsonOutput,
                               theRadio,
	                       theChannel,
	                       ensembleName,
	                       ensembleId,
	                       &firstEnsemble);

	   print_audioheader (outFile, jsonOutput);
	   for (int i = 0; i < (int)(programNames. size ()); i ++) {
	      if (is_audioService (theRadio, programNames [i])) {
	         audiodata ad;
	         dataforAudioService (theRadio,
	                              programNames [i],
	                              ad, 0);
	         print_audioService (outFile, 
                                     jsonOutput,
	                             theRadio,
	                             programNames [i],
	                             theChannel,
	                             &ad,
	                             &firstService);
	      }
	   }

	   for (int i = 0; i < (int)(programNames. size ()); i ++) {
	      if (is_dataService (theRadio, programNames [i])) {
	         if (firstTime)
	            print_dataHeader (outFile, jsonOutput);
	         firstTime	= false;
                 packetdata pd;
                 dataforDataService (theRadio,
                                        programNames [i],
                                        pd, 0);
                 if (pd. defined)
                    print_dataService (outFile,
                                       jsonOutput,
                                       theRadio,
                                       programNames [i],
                                       theChannel,
                                       0,
	                               &pd,
                                       &firstService);
                
	      }
	   }
	   
	   print_ensembleFooter (outFile, jsonOutput);

	   the_tiiHandler. print ();
#ifdef	HAVE_RTLSDR
	   if (rawDump)
	      ((rtlsdrHandler *)theDevice) -> stopDumping ();
#endif
	   theDevice	-> stopReader ();
	   the_tiiHandler. stop ();
	   dabStop (theRadio);
	   programNames. resize (0);
	   programSIds.  resize (0);
	   theChannel	= dabBand. nextChannel (theBand, theChannel);
	}

	print_fileFooter (outFile, jsonOutput);
	
	fclose (outFile);
	theDevice	-> stopReader ();
	dabStop	(theRadio);
	dabExit	(theRadio);
	delete theDevice;	
}

void    printOptions () {
        fprintf (stderr,
"                        dab-scanner options are\n\
                        -F filename      in case the output is to a file\n\
                        -j               output data in json format\n\
                        -t number        threshold for tii detection\n\
                        -D number        amount of time to look for full sync\n\
                        -M Mode          Mode is 1, 2 or 4. Default is Mode 1\n\
                        -B Band          Band is either L_BAND or BAND_III (default)\n\
	                -I number	amount of time used to gather TII data\n\
                        -C start channel the start channel, default: 5A\n\
	                -R filename	raw dump of the input data\n"
"	for hackrf:\n"
"	                  -v vgaGain\n"
"	                  -l lnaGain\n"
"	                  -p number\tppm offset\n"
"	for SDRplay:\n"
"	                  -G Gain reduction in dB (range 20 .. 59)\n"
"	                  -L lnaState (depends on model chosen)\n"
"	                  -Q autogain (default off)\n"
"	                  -p number\t ppm offset\n"
"	for rtlsdr:\n"
"	                  -G number\t	gain, range 0 .. 100\n"
"	                  -Q autogain (default off)\n"
"	                  -p number\tppm offset\n"
"	for airspy:\n"
"	                  -G number\t	gain, range 1 .. 21\n"
"	                  -b set rf bias\n"
"	                  -p number\t ppm Correction\n"
"	for limesdr:\n"
"	                  -G number\t gain\n"
"	                  -X antenna selection\n"
"	                  -C channel\n");
}

