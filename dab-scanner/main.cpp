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
#include	"band-handler.h"
#include	"service-printer.h"
#ifdef	HAVE_SDRPLAY
#include	"sdrplay-handler.h"
#elif	HAVE_AIRSPY
#include	"airspy-handler.h"
#elif	HAVE_RTLSDR
#include	"rtlsdr-handler.h"
#elif	HAVE_HACKRF
#include	"hackrf-handler.h"
#endif

#include	<atomic>
#include	<thread>

using std::cerr;
using std::endl;

void    printOptions	(void);	// forward declaration
//	we deal with some callbacks, so we have some data that needs
//	to be accessed from global contexts
static
std::atomic<bool> run;

static
void	*theRadio	= NULL;

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
void	ensemblenameHandler (std::string name, int Id, void *userData) {
	fprintf (stderr, "ensemble %s is (%X) recognized\n",
	                          name. c_str (), (uint32_t)Id);
	ensembleRecognized. store (true);
	ensembleName	= name;
	ensembleId	= Id;
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
//	fprintf (stderr, "program %s is part of the ensemble\n", s. c_str ());
}

static
void	programdataHandler (audiodata *d, void *ctx) {
	(void) *d;
	(void)ctx;
}

//
//	The function is called from within the library with
//	a string, the so-called dynamic label
static
void	dataOut_Handler (std::string dynamicLabel, void *ctx) {
	(void)ctx;
//	fprintf (stderr, "%s\n", dynamicLabel. c_str ());
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
uint8_t		theMode		= 1;
std::string	theChannel	= "5A";
std::string	startChannel	= "5A";
uint8_t		theBand		= BAND_III;
int16_t		ppmCorrection	= 0;
int		theGain		= 35;	// scale = 0 .. 100
#ifdef	HAVE_HACKRF
int		lnaGain		= 40;
int		vgaGain		= 40;
#endif
int16_t		latency		= 10;
int16_t		timeSyncTime	= 5;
int16_t		freqSyncTime	= 10;
bool		autogain	= false;
int	opt;
struct sigaction sigact;
bandHandler	dabBand;
deviceHandler	*theDevice;
bool	err;

	fprintf (stderr, "dab_scanner V 1.0alfa,\n\
	                  Copyright 2018 J van Katwijk, Lazy Chair Computing\n\	                            2018 Hayati Ayguen\n");
	timeSynced.	store (false);
	timesyncSet.	store (false);
	run.		store (false);

	if (argc == 1) {
	   printOptions ();
	   exit (1);
	}

	while ((opt = getopt (argc, argv, "F:D:d:M:B:p:G:g:QC:")) != -1) {
	   switch (opt) {
	      case 'F':
	         outFile	= fopen (optarg, "w");
	         if (outFile == NULL)
	            outFile = stderr;
	         break;

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

	      case 'p':
	         ppmCorrection	= atoi (optarg);
	         break;

#ifndef	HAVE_HACKRF
	      case 'G':
	         theGain	= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;
#else
	      case 'G':
	         lnaGain	= atoi (optarg);
	         break;

	      case 'g':
	         vgaGain	= atoi (optarg);
	         break;

#endif
	      case 'C':
	         theChannel	= std::string (optarg);
	         startChannel	= theChannel;
	         break;

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
	                                      3,
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
#elif	HAVE_HACKRF
	   theDevice	= new hackrfHandler (frequency,
	                                     ppmCorrection,
	                                     lnaGain,
	                                     vgaGain);
#endif
	}
	catch (int e) {
	   fprintf (stderr, "allocating device failed (%d), fatal\n", e);
	   exit (32);
	}
//
//	and with a sound device we can create a "backend"
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
	                           NULL,		// no mot slides
	                           NULL,		// no spectrum shown
	                           NULL,		// no constellations
	                           NULL
	                          );
	if (theRadio == NULL) {
	   fprintf (stderr, "sorry, no radio available, fatal\n");
	   exit (4);
	}

	theDevice	-> setGain (theGain);
	if (autogain)
	   theDevice	-> set_autogain (autogain);

	while (true) {
	   bool	firstTime	= true;
	   theDevice	-> stopReader ();
	   int32_t frequency =
	               dabBand. Frequency (theBand, theChannel);
	   theDevice	-> setVFOFrequency (frequency);
	   theDevice	-> restartReader ();

	   ensembleRecognized.	store (false);
	   dabReset (theRadio);
//	The device should be working right now

	   fprintf (stderr, "checking data in channel %s\n",
	                                          theChannel. c_str ());
	   timesyncSet.		store (false);
	   timeSynced. 		store (false);
	   timeSyncTime		= 4;
	   freqSyncTime		= 5;

	   while (!timesyncSet. load () && (--timeSyncTime >= 0))
	      sleep (1);

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
	                             (--freqSyncTime >= 0)) {
	       fprintf (stderr, "%d\r", freqSyncTime);
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
	   sleep (5);
//
//	print ensemble data here
	   print_ensembleData (outFile,
                               theRadio,
	                       theChannel,
	                       ensembleName,
	                       ensembleId);

	   print_audioheader (outFile);
	   for (int i = 0; i < programNames. size (); i ++) {
	      if (is_audioService (theRadio, programNames [i]. c_str ())) {
	         audiodata ad;
	         dataforAudioService (theRadio,
	                              programNames [i]. c_str (),
	                              &ad, 0);
	         print_audioService (outFile, 
	                             theRadio,
	                             programNames [i]. c_str (),
	                             &ad);
	         for (int j = 1; j < 5; j ++) {
	            packetdata pd;
	            dataforDataService (theRadio,
	                                programNames [i]. c_str (),
                                        &pd, j);
	            if (pd. defined)
	               print_dataService (outFile,
                                          theRadio,
                                          programNames [i]. c_str (),
                                          j,
	                                  &pd);
	         }
	      }
	   }

	   for (int i = 0; i < programNames. size (); i ++) {
	      if (is_dataService (theRadio, programNames [i]. c_str ())) {
	         if (firstTime)
	            print_dataHeader (outFile);
	         firstTime	= false;
	         for (int j = 0; j < 5; j ++) {
                    packetdata pd;
                    dataforDataService (theRadio,
                                        programNames [i]. c_str (),
                                        &pd, j);
                    if (pd. defined)
                       print_dataService (outFile,
                                          theRadio,
                                          programNames [i]. c_str (),
                                          j,
	                                  &pd);
                 }
	      }
	      else
	      if (is_audioService (theRadio, programNames [i]. c_str ())) {
	         for (int j = 1; j < 5; j ++) {
	            packetdata pd;
	            dataforDataService (theRadio,
	                                programNames [i]. c_str (),
                                        &pd, j);
	            if (pd. defined)
	               print_dataService (outFile,
                                          theRadio,
                                          programNames [i]. c_str (),
                                          j,
	                                  &pd);
	         }
	      }
	   }
	   theDevice	-> stopReader ();
	   dabStop (theRadio);
	   programNames. resize (0);
	   programSIds.  resize (0);
	   theChannel	= dabBand. nextChannel (theBand, theChannel);
	}

	fclose (outFile);
	theDevice	-> stopReader ();
	dabStop	(theRadio);
	dabExit	(theRadio);
	delete theDevice;	
}

void    printOptions (void) {
	fprintf (stderr,
"                          dab-scanner options are\n\
	                  -d number   amount of time to look for time synchr\n\
	                  -D number   amount of time to look for full sync\n\
	                  -M Mode     Mode is 1, 2 or 4. Default is Mode 1\n\
	                  -B Band     Band is either L_BAND or BAND_III (default)\n\
	                  -G number   gain for device (range 1 .. 100)\n\
	                  -Q          if set, set autogain for device true\n\
	                  -p number   ppm correction for the attached device\n\
	                  -F filename in case the output is to a file\n");
}

