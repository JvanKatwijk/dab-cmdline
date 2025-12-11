#
/*
 *    Copyright (C) 2025
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the 2-nd DAB scannner and borrowed
 *    from the Qt-DAB repository of the same author
 *
 *    DAB scannner is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB scanner is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB scanner; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	<stdlib.h>
#include	<unistd.h>
#include	<signal.h>
#include	<getopt.h>
#include        <cstdio>
#include	<vector>
#include	<iostream>
#include	"band-handler.h"
#include	"tii-handler.h"
#include	"scanner-printer.h"
#include	"csv-printer.h"
#include	"json-printer.h"
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
#elif	HAVE_RTL_TCP
#include	"rtl_tcp-client.h"
#elif	HAVE_SDRPLAY_V3
#include	"sdrplay-handler-v3.h"
#endif

#include	<atomic>
#include	<thread>

#include	"dab-api.h"
#include	"dab-processor.h"
using std::cerr;
using std::endl;

void    printOptions	();	// forward declaration
//	we deal with some callbacks, so we have some data that needs
//	to be accessed from global contexts
static
std::atomic<bool> run;

static
dabProcessor	*theRadio	= nullptr;

static
tiiHandler the_tiiHandler;
static
std::atomic<bool>timeSynced;

static
std::atomic<bool>ensembleRecognized;

static void sighandler (int signum) {
	fprintf (stderr, "Signal caught, terminating!\n");
	run. store (false);
}

static
void	syncSignalHandler (bool b, void *userData) {
	timeSynced.	store (b);
	(void)userData;
}

static std::string ensembleName;
static int ensembleId;

static
std::vector<ensembleDescriptor> theResult;

static
void	name_of_ensemble (const std::string &name, int Id, void *userData) {
	ensembleRecognized. store (true);
	ensembleName	= name;
	ensembleId	= Id;
	the_tiiHandler. start (Id);
}

static
void    tii_data_Handler        (tiiData *theData, void *ctx) {
	the_tiiHandler. add (*theData);
        (void)ctx;
}

typedef struct {
	std::string serviceName;
	int	serviceId;
} service;
static
std::vector<service> serviceList;

static
void	serviceName (const std::string &name, int SId,
	                                 uint16_t subChId, void *userdata) {
	service s;
	s. serviceName	= name;
	s. serviceId	= SId;
	serviceList. push_back (s);
}

static
void	theTime (int hours, int minutes, void *userData) {
}
//
int	main (int argc, char **argv) {
// Default values
#ifdef	HAVE_HACKRF
int		lnaGain		= 40;
int		vgaGain		= 40;
int		ppmOffset	= 0;
const char	*optionsString	= "JI:F:D:G:g:p:";
#elif	HAVE_LIME
int16_t		gain		= 70;
std::string	antenna		= "Auto";
const char	*optionsString	= "JI:F:D:G:g:X:";
#elif	HAVE_SDRPLAY	
int16_t		GRdB		= 30;
int16_t		lnaState	= 3;
bool		autogain	= true;
int16_t		ppmOffset	= 0;
const char	*optionsString	= "JI:F:D:G:L:Qp:";
#elif	HAVE_SDRPLAY_V3	
int16_t		GRdB		= 30;
int16_t		lnaState	= 2;
bool		autogain	= true;
int16_t		ppmOffset	= 0;
const char	*optionsString	= "JI:F:D:G:L:Qp:";
#elif	HAVE_AIRSPY
int16_t		gain		= 20;
bool		autogain	= false;
bool		rf_bias		= false;
const char	*optionsString	= "JI:F:D:G:bp:";
#elif	HAVE_RTLSDR
int16_t		gain		= 50;
bool		autogain	= false;
int16_t		ppmOffset	= 0;
int		dumpDuration	= 1;
const char	*optionsString	= "JIF:D:G:p:QT:";
#elif   HAVE_RTL_TCP
int		rtl_tcp_gain	= 50;
bool		autogain	= false;
int		rtl_tcp_ppm	= 0;
std::string	rtl_tcp_hostname	= "127.0.0.1";  // default
int32_t		rtl_tcp_basePort	= 1234;         // default
const char      *optionsString  = "JI:F:D:G:g:P:H:h:p:Q";
#endif
int	opt;
int	freqSyncTime		= 8;
int	tiiWaitTime		= 10;
struct sigaction sigact;
bandHandler	dabBand;
deviceHandler	*theDevice;
std::string	fileName = "";
bool	json		= false;

	fprintf (stdout, "dab_scanner V 3.0,\n"
	                "Copyright 2025 J van Katwijk, Lazy Chair Computing\n");
	timeSynced.	store (false);
	run.		store (false);

	if (argc == 1) {
	   printOptions ();
	   exit (1);
	}

	while ((opt = getopt (argc, argv, optionsString)) != -1) {
	   switch (opt) {
	      case 'J':
	         json		= true;
	         break;
	      case 'I':
	         tiiWaitTime	= atoi (optarg);
	         break;
	      case 'F':
	         fileName	= std::string (optarg);
	         break;
	      case 'D':
	         freqSyncTime	= atoi (optarg);
	         break;

//	device specific options

#ifdef	HAVE_RTL_TCP
	      case 'G':
	      case 'g':
		 rtl_tcp_gain	= atoi (optarg);
	         break;
	      case 'P':	
	         rtl_tcp_basePort	= atoi (optarg);
	         break;
	      case 'h':
	      case 'H':
	         rtl_tcp_hostname	= std::string (optarg);
	         break;
	      case  'p':
	         rtl_tcp_ppm	= atoi (optarg);
	         break;
	      case 'Q':
	         autogain	= true;
	         break;
#elif	HAVE_HACKRF
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

	int	defaultFrequency	= 200000;
	try {
#ifdef	HAVE_SDRPLAY
	   theDevice	= new sdrplayHandler (defaultFrequency,
	                                      ppmOffset,
	                                      GRdB,
	                                      lnaState,
	                                      autogain,
	                                      0,
	                                      0);
#elif   HAVE_SDRPLAY_V3
           theDevice    = new sdrplayHandler_v3 (defaultFrequency,
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
#elif	HAVE_RTL_TCP
	   theDevice    = new rtl_tcp_client (rtl_tcp_hostname,
                                              rtl_tcp_basePort,
                                              frequency,
                                              rtl_tcp_gain,
                                              autogain,
                                              rtl_tcp_ppm);
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
	interface. thresholdValue	= 6;
        interface. syncsignal_Handler   = syncSignalHandler;
	interface. tii_data_Handler	= tii_data_Handler;
        interface. name_of_ensemble 	= name_of_ensemble;
        interface. serviceName  	= serviceName;
	interface. timeHandler		= theTime;
//
//	and with a sound device we can create a "worker"
	theRadio	= new dabProcessor (theDevice,
	                                    &interface,
	                                    nullptr);
	if (theRadio == nullptr) {
	   fprintf (stderr, "sorry, no radio device available, fatal\n");
	   exit (4);
	}

	if (autogain)
	   theDevice	-> set_autogain (autogain);

	for (int l = 0; l < 2; l ++)
	for (auto &currFreq : dabBand. theFreqs) {
	   int	the_timeSyncTime	= 5;
	   int	the_freqSyncTime	= freqSyncTime;

	   int32_t frequency = currFreq. fKHz * 1000;
	   std::string currentChannel	= std::string (currFreq. key);
	   theDevice	-> stopReader	();
	   theRadio	-> reset ();
	   theDevice	-> restartReader (frequency);
	   ensembleRecognized.	store (false);
	   ensembleName		= "";
	   ensembleId		= 0;
	   serviceList. resize (0);
//	The device should be working right now

	   fprintf (stderr, "checking data in channel %s\n", currFreq. key);
	   timeSynced. 		store (false);

	   while (!timeSynced. load () && (--the_timeSyncTime >= 0)) {
	      sleep (1);
	   }
	   if (!timeSynced. load ()) {
	      fprintf (stderr, "no signal detected in channel %s\n", 
	                                                    currFreq. key);
	      continue;
	   }

//	we MIGHT have data here, not sure yet
	   while (!ensembleRecognized. load () &&
	                             (--the_freqSyncTime >= 0)) {
	       sleep (1);
	   }

	   if ((ensembleId == 0) || !theRadio -> syncReached ()) {
	      fprintf (stderr, "no sync reached for channel %s\n", 
	                                                currFreq. key);
	      continue;
	   }

	   int tiiTime	= tiiWaitTime;
	   while (--tiiTime > 0)
	      sleep (1);
	   fprintf (stderr, "channel %s -> EId %X\tensembleName %s\n",
	                           currFreq. key, ensembleId,
	                                        ensembleName. c_str ());
	   for (auto &serv : serviceList)
	      fprintf (stderr, "%X\t%s\n", serv. serviceId,
	                                          serv. serviceName. c_str ());
	   ensembleDescriptor ensem;
	   ensem. channel	= currFreq. key;
	   ensem. ensemble	= ensembleName;
	   ensem. ensembleId	= ensembleId;
	   ensem. snr		= theRadio	-> get_snr ();
	   for (int i = 0; i < the_tiiHandler. nrTransmitters (); i ++) {
	      cacheElement e = the_tiiHandler. deliver (i);
	      ensem. transmitterData. push_back (e);
	   }
	   for (int i = 0; i < theRadio -> get_nrComponents (); i ++) {
	      contentType c = theRadio -> content (i);
	      if (c. TMid == 0)
	         ensem. audioServices. push_back (c);
	      else
	      if (c. TMid == 3)
	         ensem. packetServices. push_back (c);
	   }
	   the_tiiHandler. stop ();
	   theResult. push_back (ensem);
	}

	fprintf (stderr, "The result has %d elements\n",
	                               (int)theResult. size ());
	theDevice	-> stopReader ();
	if (fileName == "")
	   fileName = json ? "test.json" : "test.csv";
	scannerPrinter *thePrinter;
	if (json)
	   thePrinter  = new json_printer (fileName);
	else
	   thePrinter  = new csv_printer (fileName);
	thePrinter ->  print (theResult);
	delete thePrinter;
	delete	theRadio;
	delete	theDevice;	
}

void    printOptions () {
        fprintf (stderr,
"                        dab-scanner options are\n"
"	                -J		output in json rather than csv format\n"
"	                -F filename	in case the output is to a file\n"
"	                -D number	amount of time to look for full sync\n"
"	                -I number	amount of time used to gather TII data\n"
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

