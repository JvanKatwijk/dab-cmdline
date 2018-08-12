
/*
 *    Copyright (C) 2015, 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    Copyright (C) 2018
 *    Hayati Ayguen (h_ayguen@web.de)
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
 *	your wishes. Do not take it as a definitive and "ready: program
 *	for the DAB-library
 */
#include	<unistd.h>
#include	<signal.h>
#include	<getopt.h>
#include        <cstdio>
#include        <iostream>
#include	"dab-api.h"
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

#include	<atomic>
#ifdef	DATA_STREAMER
#include	"tcp-server.h"
#endif

#include "dab_tables.h"

using std::cerr;
using std::endl;

#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>

#include <unordered_map>

/* where to output program/scan infos, when using option '-E' */
static FILE * infoStrm = stderr;

struct MyServiceData {
    MyServiceData (void) {
        SId = 0;
        gotAudio = false;
        gotAudioPacket[0] = gotAudioPacket[1] = gotAudioPacket[2] = gotAudioPacket[3] = false;
        gotPacket = false;
    }

    int SId;
    std::string programName;
    bool    gotAudio;
    audiodata   audio;
    bool    gotAudioPacket[4];  // 1 .. 4 --> 0 .. 3
    packetdata  audiopacket[4];

    bool    gotPacket;
    packetdata  packet;
};

struct MyGlobals {
    std::unordered_map<int, MyServiceData*> channels;
};

MyGlobals globals;

#define PRINT_COLLECTED_STAT_AND_TIME  0
#define PRINT_LOOPS    0
#define PRINT_DURATION 1

#define T_UNITS		"ms"
#define T_UNIT_MUL	1000
#define T_GRANULARITY	10


inline void sleepMillis(unsigned ms) {
	usleep (ms * 1000);
}

inline uint64_t currentMSecsSinceEpoch() {
struct timeval te;

	gettimeofday(&te, 0); // get current time
uint64_t milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
//	printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

uint64_t msecs_progStart = 0;

inline long sinceStart() {
    uint64_t n = currentMSecsSinceEpoch();
    return long(n - msecs_progStart);
}

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

#ifdef	DATA_STREAMER
tcpServer	tdcServer (8888);
#endif

static std::string	programName		= "Sky";
static int32_t		serviceIdentifier	= -1;

static bool scanOnly = false;
static int16_t minSNRtoExit = -32768;
static deviceHandler	*theDevice = nullptr;


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
	fprintf (stderr, "\nensemblenameHandler: '%s' ensemble (Id %X) is recognized\n\n",
	                          name. c_str (), (uint32_t)Id);
	ensembleRecognized. store (true);
}

static
void	programnameHandler (std::string s, int SId, void * userdata) {
	fprintf (stderr, "programnameHandler: '%s' (SId %X) is part of the ensemble\n", s. c_str (), SId);
    MyServiceData * d = new MyServiceData();
    d->SId = SId;
    d->programName = s;
    globals.channels[SId] = d;
}

static
void	programdataHandler (audiodata *d, void *ctx) {
	(void)ctx;
auto p = globals.channels.find (serviceIdentifier);
	if (p != globals.channels.end () && d != NULL) {
	   p -> second -> audio = *d;
	   p -> second -> gotAudio = true;
	   fprintf (stderr, "programdataHandler for SID %X called. stored audiodata\n", serviceIdentifier  );
	}
	else {
	   fprintf (stderr, "programdataHandler for SID %X called. cannot save audiodata\n", serviceIdentifier  );
	}
}

static
void programPacketHandler (packetdata *d, int subidx /* 1 .. 4*/, void * ctx) {
    (void)ctx;
    auto p = globals.channels.find(serviceIdentifier);
	if (p != globals.channels.end () && d ) {
	   p -> second -> audiopacket    [subidx-1] = *d;
	   p -> second -> gotAudioPacket [subidx-1] = true;
	   fprintf (stderr, "programPacketHandler(%d) for SID %X called. stored packetdata\n", subidx, serviceIdentifier );
	}
	else {
	   fprintf (stderr, "programPacketHandler(%d) for SID %X called. cannot save packetdata\n", subidx, serviceIdentifier );
	}
}

static
void packetdataHandler (packetdata *d, void * ctx) {
	(void)ctx;
auto p = globals.channels.find(serviceIdentifier);

	if (p != globals.channels.end() && d) {
	   p -> second -> packet = *d;
	   p -> second -> gotPacket = true;
	   fprintf (stderr, "packetdataHandler for SID %X called. stored packetdata\n", serviceIdentifier  );
	}
	else {
	   fprintf (stderr, "packetdataHandler for SID %X called. cannot save packetdata\n", serviceIdentifier  );
	}
}

//
//	The function is called from within the library with
//	a string, the so-called dynamic label
static
void	dataOut_Handler (std::string dynamicLabel, void *ctx) {
	(void)ctx;
	static std::string lastLabel;
	if ( lastLabel != dynamicLabel ) {
	   fprintf (stderr, "dataOut: dynamicLabel = '%s'\n", dynamicLabel. c_str ());
	   lastLabel = dynamicLabel;
	}
}
//
//	The function is called from the MOT handler, with
//	as parameters the filename where the picture is stored
//	d denotes the subtype of the picture 
//	typedef void (*motdata_t)(std::string, int, void *);
void	motdataHandler (std::string s, int d, void *ctx) {
	(void)s; (void)d; (void)ctx;
	fprintf (stderr, "motdataHandler: %s\n", s. c_str ());
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
static
void	pcmHandler (int16_t *buffer, int size, int rate,
	                              bool isStereo, void *ctx) {
	if (scanOnly)
	   return;
	// output rate, isStereo once
	fwrite ((void *)buffer, size, 2, stdout);
	(void)isStereo;
	(void)ctx;
}


static bool stat_gotSysData = false, stat_gotFic = false, stat_gotMsc = false;
static bool stat_everSynced = false;
static long numSnr = 0, sumSnr = 0, avgSnr = -32768;
static long numFic = 0, sumFic = 0, avgFic = 0;
static int16_t stat_minSnr = 0, stat_maxSnr = 0;
static int16_t stat_minFic = 0, stat_maxFic = 0;
static int16_t stat_minFe = 0, stat_maxFe = 0;
static int16_t stat_minRsE = 0, stat_maxRsE = 0;
static int16_t stat_minAacE = 0, stat_maxAacE = 0;


static
void	systemData (bool flag, int16_t snr, int32_t freqOff, void *ctx) {
	if (stat_gotSysData) {
	   stat_everSynced = stat_everSynced || flag;
	   stat_minSnr = snr < stat_minSnr ? snr : stat_minSnr;
	   stat_maxSnr = snr > stat_maxSnr ? snr : stat_maxSnr;
	} else {
	   stat_everSynced = flag;
	   stat_minSnr = stat_maxSnr = snr;
	}

	++numSnr;
	sumSnr += snr;
	avgSnr = sumSnr / numSnr;
	stat_gotSysData = true;

//	fprintf (stderr, "synced = %s, snr = %d, offset = %d\n",
//	                    flag? "on":"off", snr, freqOff);
}

static
void	fibQuality	(int16_t q, void *ctx) {
	if (stat_gotFic) {
	   stat_minFic = q < stat_minFic ? q : stat_minFic;
	   stat_maxFic = q > stat_maxFic ? q : stat_maxFic;
	} else {
	   stat_minFic = stat_maxFic = q;
	   stat_gotFic = true;
	}

	++numFic;
	sumFic += q;
	avgFic = sumFic / numFic;
//	fprintf (stderr, "fic quality = %d\n", q);
}

static
void	mscQuality	(int16_t fe, int16_t rsE, int16_t aacE, void *ctx) {
	if (stat_gotMsc) {
	   stat_minFe = fe < stat_minFe ? fe : stat_minFe;
	   stat_maxFe = fe > stat_maxFe ? fe : stat_maxFe;
	   stat_minRsE = rsE < stat_minRsE ? rsE : stat_minRsE;
	   stat_maxRsE = rsE > stat_maxRsE ? rsE : stat_maxRsE;
	   stat_minAacE = aacE < stat_minAacE ? aacE : stat_minAacE;
	   stat_maxAacE = aacE > stat_maxAacE ? aacE : stat_maxAacE;
	} else {
	   stat_minFe = stat_maxFe = fe;
	   stat_minRsE = stat_maxRsE = rsE;
	   stat_minAacE = stat_maxAacE = aacE;
	   stat_gotMsc = true;
 	}
//	fprintf (stderr, "msc quality = %d %d %d\n", fe, rsE, aacE);
}

static int32_t timeOut = 0;
static int32_t nextOut = 0;


void printCollectedCallbackStat (const char * txt,
	                         int out = PRINT_COLLECTED_STAT_AND_TIME ) {
	if (!out)
	   return;

	if (timeOut >= nextOut) {	// force output with nextOut = timeOut
	   nextOut = timeOut + 500;	// output every 500 ms
	   fprintf (stderr, "\n%5ld: %s:\n", sinceStart(), txt);
	   fprintf (infoStrm, "  systemData(): %s: everSynced %s, snr min/max: %d/%d. # %ld avg %ld\n"
	                 , stat_gotSysData ? "yes" : "no"
	                 , stat_everSynced ? "yes" : "no"
	                 , int(stat_minSnr), int(stat_maxSnr)
	                 , numSnr, avgSnr );
	   fprintf (infoStrm, "  fibQuality(): %s: q min/max: %d/%d. # %ld avg %ld\n"
	                 , stat_gotFic ? "yes" : "no"
	                 , int(stat_minFic), int(stat_maxFic), numFic, avgFic);

	   fprintf ( infoStrm, "  mscQuality(): %s: fe min/max: %d/%d, rsE min/max: %d/%d, aacE min/max: %d/%d\n"
	                 , stat_gotMsc ? "yes" : "no"
	                 , int(stat_minFe), int(stat_maxFe)
	                 , int(stat_minRsE), int(stat_maxRsE)
	                 , int(stat_minAacE), int(stat_maxAacE) );

	   fprintf( infoStrm, "\n");
	}
}


static
bool	repeater		= true;

void device_eof_callback(void * userData)
{
	(void)userData;
	if ( !repeater ) {
	   fprintf (stderr, "\nEnd-of-File reached, triggering termination!\n");
	   run. store (false);
	}
}



int	main (int argc, char **argv) {

msecs_progStart = currentMSecsSinceEpoch();  // for  long sinceStart()
// Default values
uint8_t		theMode		= 1;
std::string	theChannel	= "11C";
uint8_t		theBand		= BAND_III;
int16_t		ppmCorrection	= 0;
int		theGain		= 35;	// scale = 0 .. 100

int32_t		waitingTime	= 10 * T_UNIT_MUL;
int32_t         waitingTimeInit = 10 * T_UNIT_MUL;
int32_t         waitAfterEnsemble = -1;

bool		autogain	= false;
int	opt;
struct sigaction sigact;
bandHandler	dabBand;
#if	defined (HAVE_WAVFILES) || defined (HAVE_RAWFILES)
std::string	fileName;
double fileOffset = 0.0;
#elif HAVE_RTL_TCP
std::string	hostname = "127.0.0.1";		// default
int32_t		basePort = 1234;		// default
#endif
bool	err;

	fprintf (stderr, "dab_cmdline V 1.0alfa,\n \
	                  Copyright 2017 J van Katwijk/Hayati Ayguen\n");
	timeSynced.	store (false);
	timesyncSet.	store (false);
	run.		store (false);

	if (argc == 1) {
	   printOptions ();
	   exit (1);
	}

//	For file input we do not need options like Q, G and C,
//	We do need an option to specify the filename
#if	(!defined (HAVE_WAVFILES) && !defined (HAVE_RAWFILES))
    while ((opt = getopt (argc, argv, "W:A:M:B:C:P:p:G:S:E:Q")) != -1) {
#elif   HAVE_RTL_TCP
    while ((opt = getopt (argc, argv, "W:A:M:B:C:P:p:G:S:H:I:E:Q")) != -1) {
#else
    while ((opt = getopt (argc, argv, "W:A:M:B:P:p:S:F:E:Ro:")) != -1) {
#endif
	   fprintf (stderr, "opt = %c\n", opt);
	   switch (opt) {

	      case 'W':
	         waitingTimeInit = waitingTime	= int32_t(atoi (optarg));
	         fprintf(stderr, "read option -W : max time to aquire sync is %d %s\n", int(waitingTime), T_UNITS);
	         break;

	      case 'A':
	         waitAfterEnsemble = int32_t(atoi (optarg));
	         fprintf(stderr, "read option -A : additional time after sync to aquire ensemble is %d %s\n", int(waitAfterEnsemble), T_UNITS);
	         break;

	      case 'E':
	         scanOnly = true;
	         infoStrm = stdout;
	         minSNRtoExit = int16_t( atoi(optarg) );
	         fprintf(stderr, "read option -E : scanOnly .. with minSNR %d\n", minSNRtoExit);
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
#if	defined (HAVE_WAVFILES) || defined (HAVE_RAWFILES)
	      case 'F':
	         fileName	= std::string (optarg);
	         break;
	      case 'R':
	         repeater	= false;
	         break;
	      case 'o':
	         fileOffset = atof(optarg);
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
	sigaction( SIGINT, &sigact, nullptr );

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
	   theDevice	= new wavFiles (fileName, repeater, fileOffset, device_eof_callback, nullptr );
#elif	HAVE_RAWFILES
	   theDevice	= new rawFiles (fileName, repeater, fileOffset, device_eof_callback, nullptr );
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
	   fprintf (stderr, "allocating device failed (%d), fatal\n", e);
#if PRINT_DURATION
           fprintf(stderr, "\n%5ld: exiting main()\n", sinceStart());
#endif
	   exit (32);
	}
//
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
	   fprintf (stderr, "sorry, no radio available, fatal\n");
	   nextOut = timeOut;
	   printCollectedCallbackStat ("A: no radio");
#if PRINT_DURATION
           fprintf(stderr, "\n%5ld: exiting main()\n", sinceStart());
#endif
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

        bool abortForSnr = false;
        bool continueForFullEnsemble = false;

#if PRINT_LOOPS
        fprintf(stderr, "\nbefore while1: cont=%s, abort=%s, timeout=%d, waitTime=%d\n"
          , continueForFullEnsemble ? "true":"false", abortForSnr ? "true":"false"
          , int(timeOut), int(waitingTime) );
#endif
        while ( (timeOut += T_GRANULARITY) < waitingTime && !abortForSnr) {
	   if ((!ensembleRecognized. load () || continueForFullEnsemble) && timeOut > T_GRANULARITY )
	      sleepMillis(T_GRANULARITY);		// sleep (1);  // skip 1st sleep if possible
	   printCollectedCallbackStat("wait for timeSync ..");
	   if (scanOnly && numSnr >= 5 && avgSnr < minSNRtoExit ) {
	      fprintf (stderr, "abort because minSNR %d is not met. # is %ld avg %ld\n", int(minSNRtoExit), numSnr, avgSnr );
	      abortForSnr = true;
	      break;
	   }

	   if (waitingTime >= waitingTimeInit &&
	       ensembleRecognized. load ()) {
	      const bool prevContinueForFullEnsemble = continueForFullEnsemble;
	      continueForFullEnsemble = true;
	      if (waitAfterEnsemble > 0) {
	         if (!prevContinueForFullEnsemble) {  // increase waitingTime only once
	            fprintf (stderr, "t=%d: abort later because already got ensemble data.\n", timeOut);
	            waitingTime = timeOut + waitAfterEnsemble;
	            fprintf (stderr, "waitAfterEnsemble = %d > 0  ==> waitingTime = timeOut + waitAfterEnsemble = %d + %d = %d\n"
	                           , waitAfterEnsemble
	                           , timeOut, waitAfterEnsemble, waitingTime);
	         }
	      }
	      else
	      if ( waitAfterEnsemble == 0 ) {
	         fprintf (stderr, "t=%d: abort directly because already got ensemble data.\n", timeOut);
	         fprintf (stderr, "waitAfterEnsemble == 0  ==> break\n");
	         break;
	      }
	   }

#if PRINT_LOOPS
          fprintf(stderr, "in while1: cont=%s, abort=%s, timeout=%d, waitTime=%d\n"
            , continueForFullEnsemble ? "true":"false", abortForSnr ? "true":"false"
            , int(timeOut), int(waitingTime) );
#endif
	}

#if PRINT_LOOPS
        fprintf(stderr, "\nbefore while2: cont=%s, abort=%s, timeout=%d, waitTime=%d\n"
          , continueForFullEnsemble ? "true":"false", abortForSnr ? "true":"false"
          , int(timeOut), int(waitingTime) );
#endif

        while (continueForFullEnsemble &&
	       !abortForSnr && (timeOut += T_GRANULARITY) < waitingTime) {
          sleepMillis (T_GRANULARITY);
          printCollectedCallbackStat ("wait for full ensemble info..");
          if (scanOnly && numSnr >= 5 && avgSnr < minSNRtoExit ) {
            fprintf(stderr, "abort because minSNR %d is not met. # is %ld avg %ld\n", int(minSNRtoExit), numSnr, avgSnr );
            abortForSnr = true;
            break;
          }
#if PRINT_LOOPS
          fprintf(stderr, "in while2: cont=%s, abort=%s, timeout=%d, waitTime=%d\n"
            , continueForFullEnsemble ? "true":"false", abortForSnr ? "true":"false"
            , int(timeOut), int(waitingTime) );
#endif
        }

        if (!timeSynced. load ()) {
           cerr << "There does not seem to be a DAB signal here" << endl;
	   theDevice -> stopReader ();
           dabStop (theRadio);
           nextOut = timeOut;
           printCollectedCallbackStat("B: no DAB signal");
           dabExit (theRadio);
           delete theDevice;
#if PRINT_DURATION
           fprintf(stderr, "\n%5ld: exiting main()\n", sinceStart());
#endif
           exit (22);
	}

#if PRINT_LOOPS
        fprintf(stderr, "\nbefore while3: cont=%s, abort=%s, timeout=%d, waitTime=%d\n"
          , continueForFullEnsemble ? "true":"false", abortForSnr ? "true":"false"
          , int(timeOut), int(waitingTime) );
#endif
        while (!ensembleRecognized. load () && (timeOut += T_GRANULARITY) < waitingTime && !abortForSnr) {
          sleepMillis(T_GRANULARITY);
          printCollectedCallbackStat("C: collecting ensembleData ..");
          if ( scanOnly && numSnr >= 5 && avgSnr < minSNRtoExit ) {
            fprintf(stderr, "abort because minSNR %d is not met. # is %ld avg %ld\n", int(minSNRtoExit), numSnr, avgSnr );
            abortForSnr = true;
            break;
          }
#if PRINT_LOOPS
          fprintf(stderr, "in while3: cont=%s, abort=%s, timeout=%d, waitTime=%d\n"
            , continueForFullEnsemble ? "true":"false", abortForSnr ? "true":"false"
            , int(timeOut), int(waitingTime) );
#endif
        }
	fprintf (stderr, "\n");

	if (!ensembleRecognized. load ()) {
	   fprintf (stderr, "no ensemble data found, fatal\n");
	   theDevice -> stopReader ();
	   dabReset (theRadio);
	   nextOut = timeOut;
	   printCollectedCallbackStat("D: no ensembleData");
	   dabExit (theRadio);
	   delete theDevice;
#if PRINT_DURATION
           fprintf(stderr, "\n%5ld: exiting main()\n", sinceStart());
#endif
	   exit (22);
	}

        if (ensembleRecognized. load ()) {
          nextOut = timeOut;
          printCollectedCallbackStat("summmary for found ensemble", 1);
        }

	if (!scanOnly) {
	   audiodata ad;
	   run. store (true);
	   std::cerr << "we try to start program " <<
                                                 programName << "\n";
	   if (!is_audioService (theRadio, programName. c_str ())) {
	      std::cerr << "sorry  we cannot handle service " <<
	                                           programName << "\n";
	      run. store (false);
	   }
	   else {
	      dataforAudioService (theRadio, programName. c_str (), &ad, 0);
	      if (!ad. defined) {
	         std::cerr << "sorry  we cannot handle service " <<
	         programName << "\n";
	         run. store (false);
	      }
	   }

	   if (run. load ()) {
	      dabReset_msc (theRadio);
	      set_audioChannel (theRadio, &ad);
	      while (run. load ()) {
	         timeOut += T_GRANULARITY;
	         sleepMillis (T_GRANULARITY);
	         printCollectedCallbackStat("E: loading ..");
	      }
	   }
        }
        else {	// scan only
           bool gotECC = false;
           bool gotInterTabId = false;
           const uint8_t eccCode =
	               dab_getExtendedCountryCode (theRadio, &gotECC);
           const uint8_t interTabId =
	               dab_getInternationalTabId (theRadio, &gotInterTabId);

           int16_t mainId, subId, TD;
           float gps_latitude, gps_longitude;
           bool gps_success = false;
           dab_getCoordinates (theRadio,
	                       -1, -1,
	                       &gps_latitude,
	                       &gps_longitude,
	                       &gps_success,
	                       &mainId, &subId );
	   if (gps_success) {
	      for (subId = 1; subId <= 23; ++subId ) {
	         dab_getCoordinates (theRadio,
	                             mainId, subId,
	                             &gps_latitude,
	                             &gps_longitude,
	                             &gps_success,
	                             nullptr, nullptr, &TD );
	         if (gps_success)
	            fprintf (infoStrm,
	                     "\ttransmitter gps coordinate (latitude / longitude) for\ttii %04d\t=%f / %f\tTD=%d us\n",
                             mainId * 100 + subId,
	                     gps_latitude,
	                     gps_longitude, int(TD) );
	      }	// end for
	   }	// end if (gps_success)

	   for (auto & it : globals.channels ) {
	      serviceIdentifier = it.first;
	      const bool callbackDataOnly = true;
	      if (is_audioService (theRadio,
	                               it. second -> programName. c_str ()) ||
	          is_dataService  (theRadio,
	                               it. second -> programName. c_str ())) {
	         fprintf (infoStrm,
	                      "\nchecked program '%s' with SId %X\n",
		               it. second -> programName.c_str(),
	                       serviceIdentifier);

	         if (it. second -> gotAudio) {
	            audiodata * d = &(it.second->audio);
	            uint8_t countryId =
	                        (serviceIdentifier >> 12) & 0xF;  // audio
	            fprintf (infoStrm, "\taudioData:\n");
	            fprintf (infoStrm, "\t\tsubchId\t=%d\n",
	                                             int(d->subchId));
		    fprintf (infoStrm, "\t\tstartAddr\t=%d\n",
	                                             int(d->startAddr));
	            fprintf (infoStrm, "\t\tshortForm\t=%s\n",
	                                   d->shortForm ? "true":"false");
	            fprintf (infoStrm, "\t\tprotLevel\t=%d: '%s'\n",
	                                   int(d->protLevel),
	                                   getProtectionLevel (d->shortForm, d->protLevel));
	            fprintf (infoStrm, "\t\tcodeRate\t=%d: '%s'\n",
	                               int (d->protLevel),
	                               getCodeRate (d -> shortForm,
	                                            d -> protLevel));
	            fprintf (infoStrm, "\t\tlength\t=%d\n", int (d -> length));
	            fprintf (infoStrm, "\t\tbitRate\t=%d\n", int (d ->bitRate));
	            fprintf (infoStrm, "\t\tASCTy\t=%d: '%s'\n", int(d->ASCTy), getASCTy(d->ASCTy));
	            if (gotECC)
	               fprintf (infoStrm,
	                        "\t\tcountry\tECC %X, Id %X: '%s'\n", 
	                        int (eccCode), int(countryId),
	                        getCountry (eccCode, countryId));
	            fprintf (infoStrm,
	                     "\t\tlanguage\t=%d: '%s'\n",
	                     int(d->language), getLanguage(d->language));
	            fprintf (infoStrm,
	                     "\t\tprogramType\t=%d: '%s'\n",
	                     int (d -> programType),
	                     getProgramType (gotInterTabId,
	                                     interTabId, d->programType) );
                    }
                    for (int k = 0; k < 4; ++k ) {
                        if (!it. second -> gotAudioPacket [k])
                            continue;
                        packetdata *d = &(it. second -> audiopacket [k]);
                        uint8_t countryId =
	                    (serviceIdentifier >> (5 * 4)) & 0xF;     // packet
                        fprintf (infoStrm, "\taudioPacket[%d]:\n", k+1);
                        fprintf (infoStrm, "\t\tsubchId\t=%d\n",
	                                                 int (d -> subchId));
                        fprintf (infoStrm, "\t\tstartAddr\t=%d\n",
	                                         int (d -> startAddr));
                        fprintf (infoStrm, "\t\tshortForm\t=%s\n",
	                                    d->shortForm ? "true":"false");
                        fprintf (infoStrm, "\t\tprotLevel\t=%d: '%s'\n",
	                                   int (d->protLevel),
	                                    getProtectionLevel (d ->shortForm,
	                                                        d->protLevel));
                        fprintf (infoStrm, "\t\tcodeRate\t=%d: '%s'\n",
	                                    int (d->protLevel),
	                                    getCodeRate (d -> shortForm,
	                                                 d -> protLevel));
                        fprintf (infoStrm, "\t\tDSCTy\t=%d: '%s'\n",
	                                      int (d -> DSCTy),
	                                      getDSCTy (d -> DSCTy));
                        fprintf (infoStrm, "\t\tlength\t=%d\n",
	                                      int (d -> length));
                        fprintf (infoStrm, "\t\tbitRate\t=%d\n",
	                                      int (d -> bitRate));
                        fprintf (infoStrm, "\t\tFEC_scheme\t=%d: '%s'\n",
	                                   int (d -> FEC_scheme),
	                                   getFECscheme (d -> FEC_scheme));
                        fprintf (infoStrm, "\t\tDGflag\t=%d\n",
	                                   int (d -> DGflag));
                        fprintf (infoStrm, "\t\tpacketAddress\t=%d\n",
	                                   int (d -> packetAddress));
                        if (gotECC )
                            fprintf (infoStrm,
	                            "\t\tcountry\tECC %X, Id %X: '%s'\n",
	                            int (eccCode), int (countryId),
	                            getCountry (eccCode, countryId));
                        fprintf (infoStrm,
	                         "\t\tappType\t=%d: '%s'\n",
	                         int (d -> appType),
	                         getUserApplicationType (d -> appType));
                    }
                    if (it. second -> gotPacket) {
                        packetdata *d = &(it. second -> packet);
                        uint8_t countryId = (serviceIdentifier >> (5 * 4)) & 0xF;     // packet
                        fprintf(infoStrm, "\tpacket:\n");
                        fprintf(infoStrm, "\t\tsubchId\t=%d\n", int(d->subchId));
                        fprintf(infoStrm, "\t\tstartAddr\t=%d\n", int(d->startAddr));
                        fprintf(infoStrm, "\t\tshortForm\t=%s\n", d->shortForm ? "true":"false");
                        fprintf(infoStrm, "\t\tprotLevel\t=%d: '%s'\n", int(d->protLevel), getProtectionLevel(d->shortForm, d->protLevel));
                        fprintf(infoStrm, "\t\tcodeRate\t=%d: '%s'\n", int(d->protLevel), getCodeRate(d->shortForm, d->protLevel));
                        fprintf(infoStrm, "\t\tDSCTy\t=%d: '%s'\n", int(d->DSCTy), getDSCTy(d->DSCTy));
                        fprintf(infoStrm, "\t\tlength\t=%d\n", int(d->length));
                        fprintf(infoStrm, "\t\tbitRate\t=%d\n", int(d->bitRate));
                        fprintf(infoStrm, "\t\tFEC_scheme\t=%d: '%s'\n", int(d->FEC_scheme), getFECscheme(d->FEC_scheme));
                        fprintf(infoStrm, "\t\tDGflag\t=%d\n", int(d->DGflag));
                        fprintf(infoStrm, "\t\tpacketAddress\t=%d\n", int(d->packetAddress));
                        if ( gotECC )
                            fprintf(infoStrm, "\t\tcountry\tECC %X, Id %X: '%s'\n", int(eccCode), int(countryId), getCountry(eccCode, countryId));
                        fprintf(infoStrm, "\t\tappType\t=%d: '%s'\n", int(d->appType), getUserApplicationType(d->appType) );
                        fprintf(infoStrm, "\t\tis_madePublic\t=%s\n", d->is_madePublic ? "true":"false");
	         }
	      }
           }

	   nextOut = timeOut;
	   printCollectedCallbackStat("D: quit without loading");
	}

	theDevice	-> stopReader ();
	dabExit (theRadio);
	delete theDevice;

#if PRINT_DURATION
	fprintf(stderr, "\n%5ld: end of main()\n", sinceStart ());
#endif
}

void    printOptions (void) {
        fprintf (stderr,
"        dab-cmdline options are\n\
        -W number   amount of time to look for a time sync in %s\n\
        -A number   amount of time to look for an ensemble in %s\n\
        -E minSNR   activates scan mode: if set, quit after loading scan data\n\
                    also quit, if SNR is below minSNR\n\
        -M Mode     Mode is 1, 2 or 4. Default is Mode 1\n\
        -B Band     Band is either L_BAND or BAND_III (default)\n\
        -P name     program to be selected in the ensemble\n\
        -p ppmCorr  ppm correction\n"
#if	defined (HAVE_WAVFILES) || defined (HAVE_RAWFILES)
"        -F filename in case the input is from file\n"
"        -o offset   offset in seconds from where to start file playback\n"
"        -R          deactivates repetition of file playback\n"

#else
"        -C channel  channel to be used\n\
        -G Gain     gain for device (range 1 .. 100)\n\
        -Q          if set, set autogain for device true\n"
#ifdef	HAVE_RTL_TCP
"        -H hostname  hostname for rtl_tcp\n\
        -I port      port number to rtl_tcp\n"
#endif
#endif
"        -S hexnumber use hexnumber to identify program\n\n", T_UNITS, T_UNITS );
}

