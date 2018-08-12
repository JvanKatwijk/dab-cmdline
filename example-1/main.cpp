#
/*
 *    Copyright (C) 2015, 2016
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
#include	<vector>
#include	"audiosink.h"
#include	"dab-api.h"
#include	"band-handler.h"
#ifdef	HAVE_SDRPLAY
#include	"sdrplay-handler.h"
#elif	HAVE_AIRSPY
#include	"airspy-handler.h"
#elif	HAVE_RTLSDR
#include	"rtlsdr-handler.h"
#endif
#include	<atomic>

using std::cerr;
using std::endl;

//	we deal with some callbacks, so we have some data that needs
//	to be accessed from global contexts
static
std::atomic<bool> run;

static
void		*theRadio	= NULL;

static
std::atomic<bool>timeSynced;

static
std::atomic<bool>timesyncSet;

static
std::atomic<bool>ensembleRecognized;

static
audioSink	*soundOut	= NULL;

std::string	programName	= "Classic FM";
int32_t		serviceId	= -1;

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
int16_t i;
	for (std::vector<std::string>::iterator it = programNames.begin();
	             it != programNames. end(); ++it)
	   if (*it == s)
	      return;
	programNames. push_back (s);
	programSIds . push_back (SId);
	fprintf (stderr, "program %s is part of the ensemble\n", s. c_str ());
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
//	fprintf (stderr, "%s\n", dynamicLabel. c_str ());
}

static
void	bytesOut_Handler (uint8_t *data, int16_t amount, uint8_t k, void *ctx) {
	(void)data;
	(void)amount;
	(void)ctx;
}
//
//	The function is called from within the library with
//	a buffer full of PCM samples. We pass them on to the
//	audiohandler, based on portaudio.
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
int		theGain		= 35;	// scale = 0 .. 100
std::string	soundChannel	= "default";
int16_t		latency		= 10;
bool		autogain	= false;
int	opt;
struct sigaction sigact;
bandHandler	dabBand;
deviceHandler	*theDevice;

	fprintf (stderr, "dab_cmdline, \
	                  Copyright 2017 J van Katwijk, Lazy Chair Computing");
	timeSynced.	store (false);
	timesyncSet.	store (false);
	run.		store (false);

	while ((opt = getopt (argc, argv, "M:B:C:P:G:A:L:S:Q")) != -1) {
	   switch (opt) {

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
	         theChannel	= std::string (optarg);
	         break;

	      case 'P':
	         programName	= optarg;
	         break;

	      case 'p':
	         ppmCorrection	= atoi (optarg);
	         break;

	      case 'G':
	         theGain	= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
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
                 ss >> serviceId;
                 break;
              }

	      default:
	         break;
	   }
	}
//
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
//	sigaction(SIGINT, &sigact, NULL);
//	sigaction(SIGTERM, &sigact, NULL);
//	sigaction(SIGQUIT, &sigact, NULL);
	bool	err;

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
	                                     autogain,
	                                     0);
#endif
	}
	catch (int e) {
	   fprintf (stderr, "allocating device failed, fatal\n");
	   exit (32);
	}
//
//	We have a device
	soundOut	= new audioSink	(latency, soundChannel, &err);
	if (err) {
	   fprintf (stderr, "no valid sound channel, fatal\n");
	   exit (33);
	}
//
//	and a sound device
	theRadio	= dabInit (theDevice,
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
	                           NULL,	// no MOT handling
	                           NULL,	// no spectrum shown
                                   NULL,	// no constellations
	                           NULL
	                          );
	if (theRadio == NULL) {
	   fprintf (stderr, "sorry, no radio available, fatal\n");
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
	dabStartProcessing (theRadio);

	int	timeOut	= 0;
	while (!timesyncSet. load () && (++timeOut < 5))
           sleep (1);

        if (!timeSynced. load ()) {
           cerr << "There does not seem to be a DAB signal here" << endl;
	   theDevice -> stopReader ();
	   sleep (1);
	   dabExit (theRadio);
	   delete theDevice;
	   exit (22);
	}
        else
	   cerr << "there might be a DAB signal here" << endl;

	if (!ensembleRecognized. load ())
	   while (!ensembleRecognized. load () && (++timeOut < latency)) {
	      fprintf (stderr, "%d\r", latency - timeOut);
	      sleep (1);
	   }
	fprintf (stderr, "\n");

	if (!ensembleRecognized. load ()) {
	   fprintf (stderr, "no ensemble data found, fatal\n");
	   theDevice -> stopReader ();
	   sleep (1);
	   dabExit (theRadio);
	   delete theDevice;
	   exit (22);
	}

	run. store (true);
	if (serviceId != -1) 
	   programName = dab_getserviceName (serviceId, theRadio);

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

	while (run. load ())
	   sleep (1);
	theDevice	-> stopReader ();
	dabExit (theRadio);
}

