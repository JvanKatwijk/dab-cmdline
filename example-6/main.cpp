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
#include	"stdin-handler.h"
#include	<locale>
#include	<codecvt>
#include	<atomic>
#include	<string>
using std::cerr;
using std::endl;

#define	MHz(x)	(x * 1000000)

void    printOptions (void);	// forward declaration
//	we deal with callbacks from different threads. So, if you extend
//	the functions, take care and add locking whenever needed
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
audioBase	*soundOut	= NULL;

#ifdef	DATA_STREAMER
tcpServer	tdcServer (8888);
#endif

std::string	programName		= "Sky Radio";

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
void	ensemblename_Handler (const char *name, int Id, void *userData) {
	fprintf (stderr, "ensemble %s is (%X) recognized\n",
	                          name, (uint32_t)Id);
	ensembleRecognized. store (true);
}


std::vector<std::string> programNames;
std::vector<int> programSIds;

#include	<bits/stdc++.h>

std::unordered_map <int, std::string> ensembleContents;
static
void	programname_Handler (const char * s, int SId, void *userdata) {
	for (std::vector<std::string>::iterator it = programNames.begin();
	             it != programNames. end(); ++it)
	   if (*it == std::string (s))
	      return;
	ensembleContents. insert (pair <int, std::string> (SId, std::string (s)));
	programNames. push_back (std::string (s));
	programSIds . push_back (SId);
	std::cerr << "program " << s << " is part of the ensemble\n";
}

static
void	programdata_Handler (audiodata *d, void *ctx) {
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
void	dataOut_Handler (const char *dynamicLabel, void *ctx) {
	(void)ctx;
	std::cerr << dynamicLabel << "\r";
}
//
//	The function is called from the MOT handler, with
//	as parameters the filename where the picture is stored
//	d denotes the subtype of the picture
//	typedef void (*motdata_t)(std::string, int, void *);
void	motdata_Handler (uint8_t * data, int size,
	                 const char *name, int d, void *ctx) {
	(void)data; (void)size; (void)name; (void)d; (void)ctx;
//	fprintf (stderr, "plaatje %s\n", s. c_str ());
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
	(void)type;
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
std::string	soundChannel	= "default";
int16_t		latency		= 10;
int16_t		timeSyncTime	= 5;
int16_t		freqSyncTime	= 5;
int		opt;
struct sigaction sigact;
bandHandler	dabBand;
deviceHandler	*theDevice;
bool	err;

	std::cerr << "dab_cmdline example 7,\n \
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
	while ((opt = getopt (argc, argv, "D:d:M:B:P:A:L:S:F:O:R")) != -1) {
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

	      case 'P':
	         programName	= optarg;
	         break;

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

	      case 'S': {
                 std::stringstream ss;
                 ss << std::hex << optarg;
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
	try {
	   theDevice	= new stdinHandler ();
	}
  catch (std::exception& ex) {
	   std::cerr << "allocating device failed (" << e << "), fatal\n";
     printf("Exception : %s\n",ex.what());
	   exit (1);
	}
//
	if (soundOut == NULL) {	// not bound to a file?
	   soundOut	= new audioSink	(latency, soundChannel, &err);
	   if (err) {
	      std::cerr << "no valid sound channel, fatal\n";
	      exit (33);
	   }
	}
//	and with a sound device we now can create a "backend"
        API_struct interface;
        interface. dabMode      = theMode;
        interface. syncsignal_Handler   = syncsignalHandler;
        interface. systemdata_Handler   = systemData;
        interface. ensemblename_Handler = ensemblename_Handler;
        interface. programname_Handler  = programname_Handler;
        interface. fib_quality_Handler  = fibQuality;
        interface. audioOut_Handler     = pcmHandler;
        interface. dataOut_Handler      = dataOut_Handler;
        interface. bytesOut_Handler     = bytesOut_Handler;
        interface. programdata_Handler  = programdata_Handler;
        interface. program_quality_Handler              = mscQuality;
        interface. motdata_Handler      = motdata_Handler;
        interface. tii_data_Handler	= nullptr;
        interface. timeHandler	 	= nullptr;

//	and with a sound device we now can create a "backend"
	theRadio	= dabInit (theDevice,
	                           &interface,
	                           nullptr,		// no spectrum shown
	                           nullptr,		// no constellations
	                           nullptr		// Ctx
	                          );
	if (theRadio == NULL) {
	   std::cerr << "sorry, no radio available, fatal\n";
	   exit (4);
	}

	theDevice	-> restartReader (MHz (220));	// freq is dummy here
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
"                          dab-cmdline options are\n\
                          -D number   amount of time to look for an ensemble\n\
	                  -d number   seconds within a time sync should be reached\n\
                          -M Mode     Mode is 1, 2 or 4. Default is Mode 1\n\
                          -B Band     Band is either L_BAND or BAND_III (default)\n\
                          -P name     program to be selected in the ensemble\n\
                          -A name     select the audio channel (portaudio)\n\
	                  -O filename put the output into a file rather than through portaudio\n";
}
