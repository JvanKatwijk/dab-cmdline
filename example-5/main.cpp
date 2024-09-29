#
/*
 *    Copyright (C) 2015, 2016, 2017, 2024
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
#include	"audiosink.h"
#include	"filesink.h"
#include	"dab-api.h"
#include	"locking-queue.h"
#include	"includes/support/band-handler.h"
#ifdef	HAVE_SDRPLAY
#include	"sdrplay-handler.h"
#elif	HAVE_SDRPLAY_V3
#include	"sdrplay-handler-v3.h"
#elif	HAVE_AIRSPY
#include	"airspy-handler.h"
#elif	HAVE_RTLSDR
#include	"rtlsdr-handler.h"
#elif	HAVE_WAVFILES
#include	"wavfiles.h"
#elif	HAVE_RAWFILES
#include	"rawfiles.h"
#elif	HAVE_XMLFILES
#include	"xml-filereader.h"
#elif	HAVE_RTL_TCP
#include	"rtl_tcp-client.h"
#elif   HAVE_PLUTO
#include        "pluto-handler.h"
#endif

#define DATA_STREAMER true
#include	<atomic>
#include	<thread>
#ifdef	DATA_STREAMER
#include	"tcp-server.h"
#endif

using std::cerr;
using std::endl;
//
//      messages
typedef struct {
	int key;
	std::string string;
} message;

static
lockingQueue<message> messageQueue;

deviceHandler	*theDevice	= nullptr;
std::string	theChannel	= "11C";
std::string	programName	= "";

#define	S_QUIT	0100
#define	S_NEXT	0101
#define	S_UPARROW	0102
#define	S_DOWNARROW	0103

void    printOptions	();	// forward declaration
void	listener	();
void	selectNextService	();
void	startAudio	(const std::string &, audiodata *);
void	startData	(const std::string &, packetdata *);
//	we deal with some callbacks, so we have some data that needs
//	to be accessed from global contexts
static
std::atomic<bool> run;

static
void	*theRadio	= nullptr;

static
std::atomic<bool>timeSynced;

static
std::atomic<bool>timesyncSet;

static
std::atomic<bool>ensembleRecognized;

static
audioBase	*soundOut	= nullptr;

static
bool	show_dynLab	= false;
static
bool	show_mscQuality	= false;
#ifdef	DATA_STREAMER
tcpServer	tdcServer (8888);
#endif

// Relaxed matching
bool matches(const std::string& s1, const std::string& s2) {
    // Trim s1 to the length of s2 and compare
    return s1. substr (0, s2.size()) == s2;
}


static void sighandler (int signum) {
	fprintf (stderr, "Signal caught, terminating!\n");
	run. store (false);
}

static
void	syncsignal_Handler (bool b, void *userData) {
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
void	ensemblename_Handler (const char * name, int Id, void *userData) {
	fprintf (stderr, "ensemble %s is (%X) recognized\n",
	                          name, (uint32_t)Id);
	ensembleRecognized. store (true);
}

std::vector<std::string> programNames;
std::vector<int> programSIds;

static
void	programname_Handler (const char *s, int SId, void *userdata) {
	for (std::vector<std::string>::iterator it = programNames.begin();
	             it != programNames. end(); ++it)
	   if (*it == std::string (s))
	      return;
	programNames. push_back (std::string (s));
	programSIds . push_back (SId);
	fprintf (stderr, "program %s is part of the ensemble\n", s);
}

static
void	programdata_Handler (audiodata *d, void *ctx) {
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
void	dataOut_Handler (const char * dynamicLabel, void *ctx) {
	(void)ctx;
	if (show_dynLab)
	   fprintf (stderr, "%s\n", dynamicLabel);
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
	   localBuf [8 + i] = data[i];
	fprintf (stderr, "tdcServer::out [%d]\n",amount+8);
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
	if (show_mscQuality)
	   fprintf (stderr, "msc quality = %d %d %d\n", fe, rsE, aacE);
}

int	main (int argc, char **argv) {
// Default values
std::string	optionsString	= "";
uint8_t		theMode		= 1;
uint8_t		theBand		= BAND_III;
int16_t		ppmCorrection	= 0;
#if	defined (HAVE_SDRPLAY) || defined (HAVE_SDRPLAY_V3)
int16_t		GRdB		= 30;
int16_t		lnaState	= 3;
#endif
int		theGain		= 35;	// scale = 0 .. 100
std::string	soundChannel	= "default";
int16_t		latency		= 10;
int16_t		timeSyncTime	= 5;
int16_t		freqSyncTime	= 10;
int16_t		progSyncTime	= 10;
bool		autogain	= false;
int	opt;
struct sigaction sigact;
bandHandler	dabBand;
#if defined (HAVE_WAVFILES) || defined (HAVE_RAWFILES) || defined (HAVE_XMLFILES)
	optionsString		= "D:d:M:B:P:A:L:S:F:O:";
std::string	fileName;
bool            repeater        = true;
#elif HAVE_RTL_TCP
std::string	hostname = "127.0.0.1";		// default
int32_t		basePort = 1234;		// default
	optionsString	="D:d:M:B:P:A:L:S:F:OR:";
#elif   HAVE_PLUTO
int16_t         gain            = 60;
bool            autogain        = true;
const char      *optionsString  = "T:D:d:M:B:P:O:A:C:G:Q";
#endif
bool	err;

	fprintf (stderr, "dab_cmdline V 1.0 example 5,\n \
	                  Copyright 2017 J van Katwijk, Lazy Chair Computing\n");
	if (optionsString == "")
	   optionsString = "D:d:M:B:C:P:G:A:L:S:H:I:QO:";
	timeSynced.	store (false);
	timesyncSet.	store (false);
	run.		store (false);

	if (argc == 1) {
	   printOptions ();
	   exit (1);
	}

//	For file input we do not need options like Q, G and C,
//	We do need an option to specify the filename
	while ((opt = getopt (argc, argv, optionsString. c_str ())) != -1) {
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

	      case 'p':
	         ppmCorrection	= atoi (optarg);
	         break;

#if (defined (HAVE_WAVFILES) || defined (HAVE_RAWFILES) || defined (HAVE_XMLFILES))
	      case 'F':
	         fileName	= std::string (optarg);
	         break;
	      case 'R':
	         repeater	= false;
	         break;
#else
	      case 'C':
	         theChannel	= std::string (optarg);
	         break;
#endif
#if	defined (HAVE_SDRPLAY) || defined (HAVE_SDRPLAY_V3)
	      case 'G':
	         GRdB		= atoi (optarg);
	         break;

	      case 'L':
	         lnaState	= atoi (optarg);
	         break;

#else
	      case 'G':
	         theGain	= atoi (optarg);
	         break;

	      case 'L':
	         latency	= atoi (optarg);
	         break;

	      case 'Q':
	         autogain	= true;
	         break;
#endif

#ifdef	HAVE_RTL_TCP
	      case 'H':
	         hostname	= std::string (optarg);
	         break;

	      case 'I':
	         basePort	= atoi (optarg);
	         break;
#endif
	      case 'O':
	         soundOut	= new fileSink (std::string (optarg), &err);
	         if (!err) {
	            fprintf (stderr, "sorry, could not open file\n");
	            exit (32);
	         }
	         break;

	      case 'A':
	         soundChannel	= optarg;
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
	                                      GRdB,
	                                      lnaState,
	                                      autogain,
	                                      0,
	                                      0);
#elif	HAVE_SDRPLAY_V3
	   theDevice	= new sdrplayHandler_v3 (frequency,
	                                      ppmCorrection,
	                                      GRdB,
	                                      lnaState,
	                                      autogain,
	                                      0,
	                                      0);
#elif	HAVE_AIRSPY
	   theDevice	= new airspyHandler (frequency,
	                                     ppmCorrection,
	                                     theGain, false);
#elif	HAVE_RTLSDR
	   theDevice	= new rtlsdrHandler (frequency,
	                                     ppmCorrection,
	                                     theGain,
	                                     autogain);
#elif   HAVE_PLUTO
           theDevice    = new plutoHandler  frequency,
	                                     theGain, autogain);
#elif	HAVE_WAVFILES
	   theDevice	= new wavFiles (fileName);
#elif	HAVE_RAWFILES
	   theDevice	= new rawFiles (fileName);
#elif	HAVE_XMLFILES
	   theDevice	= new xml_fileReader (fileName, repeater);
#elif	HAVE_RTL_TCP
	   theDevice	= new rtl_tcp_client (hostname,
	                                      basePort,
	                                      frequency,
	                                      theGain,
	                                      autogain,
	                                      ppmCorrection);
#endif

	}
	catch (std::exception& ex) {
	   std::cerr << "allocating device failed (" << ex. what () << "), fatal\n";
	   exit (1);
	}
//
	if (soundOut == nullptr) {	// not bound to a file?
	   soundOut	= new audioSink	(latency, soundChannel, &err);
	   if (err) {
	      fprintf (stderr, "no valid sound channel\n");
	      exit (33);
	   }
	}
//
//	and with a sound device we can create a "backend"
	API_struct interface;
        interface. dabMode		= theMode;
        interface. syncsignal_Handler   = syncsignal_Handler;
        interface. systemdata_Handler   = systemData;
        interface. ensemblename_Handler = ensemblename_Handler;
        interface. programname_Handler  = programname_Handler;
        interface. fib_quality_Handler  = fibQuality;
        interface. audioOut_Handler     = pcmHandler;
        interface. dataOut_Handler      = dataOut_Handler;
        interface. bytesOut_Handler     = bytesOut_Handler;
        interface. programdata_Handler  = programdata_Handler;
        interface. program_quality_Handler              = mscQuality;
        interface. motdata_Handler      = nullptr;
        interface. tii_data_Handler     = nullptr;;
        interface. timeHandler		= nullptr;

	theRadio	= dabInit (theDevice,
	                           &interface,
	                           nullptr,		// no spectrum shown
	                           nullptr,		// no constellations
	                           nullptr
	                          );
	if (theRadio == nullptr) {
	   fprintf (stderr, "sorry, no radio available, fatal\n");
	   exit (4);
	}

	theDevice	-> setGain (theGain);
	if (autogain)
	   theDevice	-> set_autogain (autogain);
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

	while (!ensembleRecognized. load () &&
	                             (--freqSyncTime >= 0)) {
	    fprintf (stderr, "%d\r", freqSyncTime);
	    sleep (1);
	}
	fprintf (stderr, "\n");

	if (!ensembleRecognized. load ()) {
	   fprintf (stderr, "no ensemble data found, fatal\n");
	   theDevice -> stopReader ();
	   sleep (1);
	   dabStop	(theRadio);
	   dabExit	(theRadio);
	   delete theDevice;
	   exit (22);
	}

	if (programNames. size () == 0) {	// ensemble with no names
	   fprintf (stderr, "No services found, fatal\n");
	   theDevice -> stopReader ();
	   sleep (1);
	   dabStop	(theRadio);
	   dabExit	(theRadio);
	   delete theDevice;
	   exit (23);
	}
	   
	run. store (true);
	std::thread keyboard_listener = std::thread (&listener);

// Search Ensemble for selected programName
        audiodata ad;
	packetdata pd;
	if (programName == "")
	   programName = programNames [0];
	bool nameFound	= false;
	while ((!ad. defined && !pd. defined) && (--progSyncTime >= 0)) {
	   for (uint8_t i = 0; i < programNames. size (); i ++) {
	      if (matches (programNames [i], programName)) {
	         programName = programNames [i];
	         nameFound = true;
	         fprintf (stderr,
	                  "we now try to start program %s \n",
	                          programName. c_str ());
	         if (is_audioService (theRadio, programName. c_str ())) { 
	            startAudio (programName, &ad);
	         }
	         else
	         if (is_dataService (theRadio, programName. c_str ())) {
	            startData (programName, &pd);
	         }
	         else {
		    fprintf (stderr,"Should not happen");
	         }
		 if (!ad. defined && !pd. defined) {
		    std::cerr << "sorry  we cannot handle service " << programName << "\n";
	      	    sighandler (9);
		 }
	         break;
	      }
	   }
	   if (!nameFound)
	      programName = programNames [0];
	}
	
	while (run. load ()) {
	   message m;
	   while (!messageQueue. pop (10000, &m));
	   switch (m. key) {
	      case S_NEXT:
	         selectNextService ();
	         break;
	      case S_QUIT:
	         run. store (false);
	         break;
	      default:
	         break;
	   }
	}
	theDevice	-> stopReader ();
	keyboard_listener. join ();
	dabStop	(theRadio);
	dabExit	(theRadio);
	delete theDevice;
	delete soundOut;
	throw (1);
}

void    printOptions () {
	fprintf (stderr,
"                          dab-cmdline options are\n\
	                  -W number   amount of time to look for an ensemble\n\
	                  -M Mode     Mode is 1, 2 or 4. Default is Mode 1\n\
	                  -B Band     Band is either L_BAND or BAND_III (default)\n\
	                  -P name     program to be selected in the ensemble\n\
	                  -C channel  channel to be used\n\
	                  -G Gainreduction  for SDRPLAY (range 20 .. 59)\n\
	                  -L lnaState for SDRplay\n\
	                  -G Gain     gain for device (range 1 .. 100)\n\
	                  -L number   latency for audiobuffer\n\
	                  -Q          if set, set autogain for device true\n\
	                  -F filename in case the input is from file\n\
	                  -A name     select the audio channel (portaudio)\n\
	                  -S hexnumber use hexnumber to identify program\n\n\
	                  -O filename put the output into a file rather than through portaudio\n");
#ifdef HAVE_RTL_TCP
	fprintf (stderr,
"\n                          RTL_TCP server options\n\
			  -H IP adress\n\
	                  -I Port to connect to\n");

#endif
}

void	selectNextService	() {
int16_t	foundIndex	= -1;

	for (int i = 0; i < programNames. size (); i ++) {
	   if (matches (programNames [i], programName)) {
	      if (i == programNames. size () - 1)
	         foundIndex = 0;
	      else
	         foundIndex = i + 1;
	      break;
	   }
	}

	if (foundIndex == -1) {
	   fprintf (stderr, "system error\n");
	   sighandler (9);
	   exit (1);
	}

	audiodata ad;
	packetdata pd;
	while (!is_audioService (theRadio,
                                 programNames [foundIndex]. c_str ()) &&
	       !is_dataService (theRadio,
                                 programNames [foundIndex]. c_str ()))
	   foundIndex = (foundIndex + 1) % programNames. size ();

	programName	= programNames [foundIndex];
	fprintf (stderr, "we now try to start program %s\n",
	                                         programName. c_str ());

	if (is_audioService (theRadio, programName. c_str ())) {
	   startAudio (programName, &ad);
	}
	else
	if (is_dataService (theRadio, programName. c_str ())) {
	   startData (programName, &pd);
	}
}

#define	MAX_STRING_SIZE	100
void	listener	() {
char input [MAX_STRING_SIZE] = {0};

	while (run. load ()) {
	   message m;
	   fgets (input, sizeof (input), stdin);
	   int inputLen	= strlen (input);
	   for (int i = 0; i < inputLen; i ++) {
	      switch (input [i]) {
	         case '\n':
	            m.key = S_NEXT;
	            m. string = "";
	            messageQueue. push (m);
	            i = inputLen;	// leave the loop
	            break;
	         case 'x':
	         case 'q':
	            m. key = S_QUIT;
	            m. string = "";
	            messageQueue. push (m);
	            return;
	         case 'a':
	            show_dynLab = !show_dynLab;
	            i = inputLen;	// leave the loop
	            break;
	         case 's':
	            show_mscQuality = !show_mscQuality;
	            i	= inputLen;
	            break;
	         default:;
	           i = inputLen;
//	         fprintf (stderr, "unidentified %d (%c)\n", t, t);
	      }
	   }
	}
}

void	startAudio (const std::string &serviceName, audiodata *ad) {
	dataforAudioService (theRadio, serviceName. c_str (), ad, 0);
	dabReset_msc (theRadio);
	set_audioChannel (theRadio, ad);
}

void	startData (const std::string &serviceName, packetdata *pd) {
	dataforDataService (theRadio, serviceName. c_str (), pd, 0);
	dabReset_msc (theRadio);
	set_dataChannel (theRadio, pd);
}

