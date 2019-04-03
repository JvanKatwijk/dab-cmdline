#
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
 *	your wishes. Do not take it as a definitive and "ready" program
 *	for the DAB-library
 */
#include	<unistd.h>
#include	<signal.h>
#include	<getopt.h>
#include        <cstdio>
#include        <iostream>
#include	"dab-processor.h"
#include	"dab-api.h"
#include	"includes/support/band-handler.h"
#ifdef	HAVE_SDRPLAY
#include	"sdrplay-handler.h"
#elif	HAVE_AIRSPY
#include	"airspy-handler.h"
#elif	defined(HAVE_RTLSDR)
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
#include <map>
#include <vector>
#include <algorithm>

#define LOG_EX_TII_SPECTRUM		0
#define MAX_EX_TII_BUFFER_SIZE		16
#define PRINT_COLLECTED_STAT_AND_TIME  0
#define PRINT_LOOPS    0
#define PRINT_DURATION 1

#define T_UNITS		"ms"
#define T_UNIT_MUL	1000
#define T_GRANULARITY	10

#define ENABLE_FAST_EXIT 1

std::string prepCsvStr( const std::string & s ) {
	std::string r = "\"";
	std::string c = s;
	std::replace( c.begin(), c.end(), '"', '\'' );
	std::replace( c.begin(), c.end(), ',', ';' );
	r += c + "\"";
	return r;
}


/* where to output program/scan infos, when using option '-E' */
static FILE * infoStrm = stderr;

class MyServiceData {
public:
	MyServiceData (int SId, std::string programName ) {
	   this	-> SId		= SId;
	   this -> programName	= programName;
	   gotAudio		= false;
	   gotAudioPacket [0]	= false;
	   gotAudioPacket [1]	= false;
	   gotAudioPacket [2]	= false;
	   gotAudioPacket [3]	= false;
	   gotPacket		= false;
	}

	int		SId;
	std::string	programName;
	bool		gotAudio;
	audiodata	audio;
	bool		gotAudioPacket [4];  // 1 .. 4 --> 0 .. 3
	packetdata	audiopacket [4];
	bool		gotPacket;
	packetdata	packet;
};

struct MyGlobals {
	std::unordered_map<int, MyServiceData*> channels;
};

MyGlobals globals;


struct ExTiiInfo {
	ExTiiInfo (void):
	   tii (100),
	   numOccurences(0),
	   maxAvgSNR (-200.0F), maxMinSNR (-200.0F), maxNxtSNR (-200.0F) {
	}

	int	tii;
	int	numOccurences;
	float	maxAvgSNR;
	float	maxMinSNR;
	float	maxNxtSNR;

	bool operator < (const ExTiiInfo &b ) const {
	   const ExTiiInfo &a = *this;
	   const int minSNR_a = int ((a. maxMinSNR < 0.0F ?
	                              -0.5F : 0.5F ) + a. maxMinSNR * 10.0F);
	   const int minSNR_b = int ((b. maxMinSNR < 0.0F ?
	                              -0.5F : 0.5F ) + b.maxMinSNR * 10.0F);
	   if (minSNR_a != minSNR_b)
	      return (minSNR_a > minSNR_b);
	   const int avgSNR_a = int ((a. maxAvgSNR < 0.0F ?
	                              -0.5F : 0.5F ) + a.maxAvgSNR * 10.0F);
	   const int avgSNR_b = int ((b. maxAvgSNR < 0.0F ?
	                              -0.5F : 0.5F ) + b.maxAvgSNR * 10.0F);
	   return (avgSNR_a > avgSNR_b);
	}
};

std::map <int, int> tiiMap;
std::map <int, ExTiiInfo> tiiExMap;
int numAllTii = 0;



#if ENABLE_FAST_EXIT
  #define FAST_EXIT( N )  exit( N )
#else
  #define FAST_EXIT( N )  do { } while (0)
#endif

#if PRINT_DURATION
  #define FMT_DURATION  "%5ld: "
  #define SINCE_START   , sinceStart ()
#else
  #define FMT_DURATION  ""
  #define SINCE_START   
#endif


inline void sleepMillis(unsigned ms) {
	usleep (ms * 1000);
}

inline uint64_t currentMSecsSinceEpoch() {
struct timeval te;

	gettimeofday (&te, 0); // get current time
	uint64_t milliseconds = te.tv_sec*1000LL +
	                          te.tv_usec/1000; // calculate milliseconds
	return milliseconds;
}

uint64_t msecs_progStart = 0;

inline long sinceStart (void) {
uint64_t n = currentMSecsSinceEpoch();

	return long (n - msecs_progStart);
}

void    printOptions (void);	// forward declaration
//	we deal with callbacks from different threads. So, if you extend
//	the functions, take care and add locking whenever needed
static
std::atomic<bool> run;

static
dabProcessor	*theRadio	= NULL;

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

static std::string	ensembleName;
static uint32_t		ensembleIdentifier	= -1;

static bool		scanOnly = false;
static int16_t		minSNRtoExit = -32768;
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
void	ensemblenameHandler (std::string name, int EId, void *userData) {
	fprintf (stderr,
	         "\n" FMT_DURATION "ensemblenameHandler: '%s' ensemble (EId %X) is recognized\n\n"
	         SINCE_START , name. c_str (), (uint32_t)EId);
	ensembleName = name;
	ensembleRecognized. store (true);
}

static
void	programnameHandler (std::string s, int SId, void * userdata) {
	fprintf (stderr,
	         "programnameHandler: '%s' (SId %X) is part of the ensemble\n",
	                          s. c_str (), SId);
	MyServiceData * d = new MyServiceData (SId, s);
	globals. channels [SId] = d;
}

static
void	programdataHandler (audiodata *d, void *ctx) {
auto	p = globals.channels.find (serviceIdentifier);

	(void)ctx;
	if (p != globals.channels.end () && d != NULL) {
	   p -> second -> audio		= *d;
	   p -> second -> gotAudio	= true;
	   fprintf (stderr, "programdataHandler for SID %X called. stored audiodata\n", serviceIdentifier  );
	}
	else {
	   fprintf (stderr, "programdataHandler for SID %X called. cannot save audiodata\n", serviceIdentifier  );
	}
}

static
void	programPacketHandler (packetdata *d,
	                      int subidx /* 1 .. 4*/, void * ctx) {
auto p = globals.channels.find(serviceIdentifier);

	(void)ctx;
	if (p != globals.channels.end () && d ) {
	   p -> second -> audiopacket    [subidx - 1] = *d;
	   p -> second -> gotAudioPacket [subidx - 1] = true;
	   fprintf (stderr,
	            "programPacketHandler(%d) for SID %X called. stored packetdata\n",
	             subidx, serviceIdentifier );
	}
	else {
	   fprintf (stderr, "programPacketHandler(%d) for SID %X called. cannot save packetdata\n", subidx, serviceIdentifier );
	}
}

static
void	packetdataHandler (packetdata *d, void * ctx) {
auto	p = globals. channels. find (serviceIdentifier);

	(void)ctx;
	if (p != globals. channels.end () && d) {
	   p -> second -> packet	= *d;
	   p -> second -> gotPacket	= true;
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
static std::string lastLabel;

	(void)ctx;
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
//	fprintf (stderr, "motdataHandler: %s\n", s. c_str ());
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
	fwrite ((void *)buffer, 2, size, stdout);
	(void)isStereo;
	(void)ctx;
}


static bool	stat_gotSysData = false,
		stat_gotFic	= false,
		stat_gotMsc	= false;
static bool	stat_everSynced = false;
static long	numSnr = 0,
		sumSnr = 0,
		avgSnr = -32768;
static long	numFic = 0,
		sumFic = 0,
		avgFic = 0;
static int16_t	stat_minSnr = 0,
		stat_maxSnr = 0;
static int16_t	stat_minFic = 0,
		stat_maxFic = 0;
static int16_t	stat_minFe = 0,
		stat_maxFe = 0;
static int16_t	stat_minRsE = 0,
		stat_maxRsE = 0;
static int16_t	stat_minAacE = 0,
		stat_maxAacE = 0;


static
void	systemData (bool flag, int16_t snr, int32_t freqOff, void *ctx) {
	if (stat_gotSysData) {
	   stat_everSynced	= stat_everSynced || flag;
	   stat_minSnr		= snr < stat_minSnr ? snr : stat_minSnr;
	   stat_maxSnr		= snr > stat_maxSnr ? snr : stat_maxSnr;
	} else {
	   stat_everSynced	= flag;
	   stat_minSnr		= stat_maxSnr = snr;
	}

	numSnr ++;
	sumSnr		+= snr;
	avgSnr		= sumSnr / numSnr;
	stat_gotSysData = true;
}

static
void	tii (int16_t mainId, int16_t subId, uint32_t tii_num, void *ctx) {

	numAllTii ++;
	if (mainId >= 0) {
	   int combinedId	= mainId * 100 + subId;
	   fprintf (stderr, "tii, %d, %u\n", combinedId, tii_num );
	   if (scanOnly)
	      tiiMap [combinedId] ++;
	}
}


#if LOG_EX_TII_SPECTRUM
static FILE * powerFile = nullptr;
static float bufferedP[MAX_EX_TII_BUFFER_SIZE][2048];
static int numBufferedP = 0;
static int gPavg_T_u = 0;
#endif

static
void	tiiEx (int numOut, int *outTii,
	                   float *outAvgSNR,
	                   float *outMinSNR,
	                   float *outNxtSNR,
	                   unsigned numAvg,
	                   const float *Pavg,
	                   int Pavg_T_u,
	                   void *ctx) {
int i;

	if (numOut == 0)
	   return;

	for (i = 0; i < numOut; i++) {
	   numAllTii++;
	   fprintf (stderr,
	            "%s no %d: %d (avg %.1f dB, min %.1f, next %.1f dB, # %u)",
	            (i==0 ? "tii:":","),
	            i + 1,
	            outTii [i],
	            outAvgSNR [i],
	            outMinSNR [i],
	            outNxtSNR [i], numAvg);
	}
	if (scanOnly) {
	   fprintf(stderr, "  =>  ");
	   for (i = 0; i < numOut; i++) {
	      ExTiiInfo & ei = tiiExMap [outTii [i]];
	      ei. tii = outTii[i];
	      ei. numOccurences ++;
	      if (outAvgSNR [i] > ei. maxAvgSNR)
	         ei. maxAvgSNR = outAvgSNR [i];
	      if (outMinSNR [i] > ei. maxMinSNR)
	         ei. maxMinSNR = outMinSNR [i];
	      if (outNxtSNR [i] > ei. maxNxtSNR)
	         ei. maxNxtSNR = outNxtSNR [i];
	      fprintf (stderr,
	               "# %d (%d)%s", ei.numOccurences, outTii[i], (i==(numOut -1) ? "\n":", ") );
	   }

#if LOG_EX_TII_SPECTRUM
	   if (Pavg &&  (numBufferedP < MAX_EX_TII_BUFFER_SIZE)) {
	      for (int i = 0; i < Pavg_T_u; i ++)
	         bufferedP [numBufferedP][i] = Pavg [i];
	      gPavg_T_u = Pavg_T_u;
	      numBufferedP ++;
	   }
#endif
	} else {
	   fprintf(stderr, "\n");
	}
}

#if LOG_EX_TII_SPECTRUM

static void	writeTiiExBuffer (void) {

	if (!powerFile) {
	   powerFile = fopen ("power.csv", "w");
	}
	if (powerFile) {
	   if (!numBufferedP) {
	      fprintf (powerFile, "#0\n");
	   } else {
	      for (int i = 0; i < gPavg_T_u; i ++) {
	         int j = (i + gPavg_T_u / 2) % gPavg_T_u;	// 1024 .. 2048, 0 .. 1023
	         int k = (j < gPavg_T_u / 2 ) ? j : ( j - gPavg_T_u);	// -1024 .. +1024
	         fprintf (powerFile, "%d, ", k);
	         for (int col = 0; col < numBufferedP; ++col) {
	            float level = 10.0 * log10 (bufferedP [col][j]);
	            fprintf (powerFile, "%f,", level);
	         }
	         fprintf (powerFile, "\n");
	      }
	   }
	   fflush (powerFile);
	   fclose (powerFile);
	}
}

#endif


static
void	fibQuality	(int16_t q, void *ctx) {

	if (stat_gotFic) {
	   stat_minFic = q < stat_minFic ? q : stat_minFic;
	   stat_maxFic = q > stat_maxFic ? q : stat_maxFic;
	} else {
	   stat_minFic = stat_maxFic = q;
	   stat_gotFic = true;
	}

	numFic ++;
	sumFic += q;
	avgFic = sumFic / numFic;
}

static
void	mscQuality	(int16_t fe, int16_t rsE, int16_t aacE, void *ctx) {
	if (stat_gotMsc) {
	   stat_minFe	= fe < stat_minFe ? fe : stat_minFe;
	   stat_maxFe	= fe > stat_maxFe ? fe : stat_maxFe;
	   stat_minRsE	= rsE < stat_minRsE ? rsE : stat_minRsE;
	   stat_maxRsE	= rsE > stat_maxRsE ? rsE : stat_maxRsE;
	   stat_minAacE = aacE < stat_minAacE ? aacE : stat_minAacE;
	   stat_maxAacE = aacE > stat_maxAacE ? aacE : stat_maxAacE;
	} else {
	   stat_gotMsc	= true;
	   stat_minFe	= stat_maxFe = fe;
	   stat_minRsE	= stat_maxRsE = rsE;
	   stat_minAacE = stat_maxAacE = aacE;
 	}
}

static int32_t timeOut = 0;
static int32_t nextOut = 0;


void	printCollectedCallbackStat (const char * txt,
	                            int out = PRINT_COLLECTED_STAT_AND_TIME ) {
	if (out == 0)
	   return;

	if (timeOut >= nextOut) {	// force output with nextOut = timeOut
	   nextOut = timeOut + 500;	// output every 500 ms
	   fprintf (stderr, "\n" FMT_DURATION "%s:\n" SINCE_START , txt);
	   fprintf (infoStrm, "  systemData(): %s: everSynced %s, snr min/max: %d/%d. # %ld avg %ld\n"
	            , stat_gotSysData ? "yes" : "no"
	            , stat_everSynced ? "yes" : "no"
	            , int (stat_minSnr)
	            , int (stat_maxSnr)
	            , numSnr
	            , avgSnr
	          );
	   fprintf (infoStrm, "  fibQuality (): %s: q min/max: %d/%d. # %ld avg %ld\n"
	            , stat_gotFic ? "yes" : "no"
	            , int (stat_minFic), int (stat_maxFic), numFic, avgFic);

	   fprintf (infoStrm,
	            "  mscQuality(): %s: fe min/max: %d/%d, rsE min/max: %d/%d, aacE min/max: %d/%d\n"
	            , stat_gotMsc ? "yes" : "no"
	            , int (stat_minFe), int (stat_maxFe)
	            , int (stat_minRsE), int (stat_maxRsE)
	            , int (stat_minAacE), int(stat_maxAacE)
	           );

	   fprintf (infoStrm, "\n");
	}
}

static
bool	repeater		= false;

void device_eof_callback (void * userData) {
	(void)userData;
	if (!repeater ) {
	   fprintf (stderr, "\nEnd-of-File reached, triggering termination!\n");
	   run. store (false);
	   exit (30);
	}
}

int	main (int argc, char **argv) {
// Default values
uint8_t		theMode		= 1;
std::string	theChannel	= "11C";
uint8_t		theBand		= BAND_III;
int16_t		ppmCorrection	= 0;
int		theGain		= 35;	// scale = 0 .. 100

uint32_t	waitingTime	= 10 * T_UNIT_MUL;
uint32_t        waitingTimeInit = 10 * T_UNIT_MUL;
int32_t         waitAfterEnsemble = -1;

bool		autogain	= false;
bool		printAsCSV	= false;
int		tii_framedelay	= 10;
float		tii_alfa	= 0.9F;
int		tii_resetFrames	= 10;
bool		useExTii	= false;
const char	* rtlOpts	= NULL;

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
#if defined(HAVE_RTLSDR)
int		deviceIndex = 0;
const char	* deviceSerial = nullptr;
#endif

bool	err;

	msecs_progStart = currentMSecsSinceEpoch ();  // for  long sinceStart()
	fprintf (stderr, "dab_cmdline example-10 V 1.0alfa,\n \
	                  Copyright 2018 Hayati Ayguen/Jan van Katwijk\n");
	timeSynced.	store (false);
	timesyncSet.	store (false);
	run.		store (false);

	if (argc == 1) {
	   printOptions ();
	   exit (1);
	}

//	For file input we do not need options like Q, G and C,
//	We do need an option to specify the filename
#if	defined (HAVE_WAVFILES) || defined (HAVE_RAWFILES)
	#define FILE_OPTS		"F:Ro:"
	#define NON_FILE_OPTS
	#define RTL_TCP_OPTS
	#define RTLSDR_OPTS
#else
	#define FILE_OPTS
	#define NON_FILE_OPTS	"C:G:Q"
	#if   defined(HAVE_RTLSDR)
		#define RTLSDR_OPTS		"d:s:"
	#else
		#define RTLSDR_OPTS
	#endif
	#ifdef	HAVE_RTL_TCP
		#define RTL_TCP_OPTS	"H:I:"
	#else
		#define RTL_TCP_OPTS
	#endif
#endif

	while ((opt = getopt (argc, argv, "W:A:M:B:P:p:S:E:ct:a:r:xO:" FILE_OPTS NON_FILE_OPTS RTLSDR_OPTS RTL_TCP_OPTS )) != -1) {
	   fprintf (stderr, "opt = %c\n", opt);
	   switch (opt) {
	      case 'W':
	         waitingTimeInit = waitingTime	= int32_t(atoi (optarg));
	         fprintf (stderr,
	                 "read option -W : max time to aquire sync is %d %s\n",
	                  int32_t (waitingTime), T_UNITS);
	         break;

	      case 'A':
	         waitAfterEnsemble = int32_t(atoi (optarg));
	         fprintf (stderr,
	                  "read option -A : additional time after sync to aquire ensemble is %d %s\n",
	                  int32_t (waitAfterEnsemble),
	                  T_UNITS);
	         break;

	      case 'E':
	         scanOnly	= true;
	         infoStrm	= stdout;
	         minSNRtoExit	= int16_t (atoi(optarg));
	         fprintf (stderr,
	                  "read option -E : scanOnly .. with minSNR %d\n",
	                  minSNRtoExit);
	         break;

	      case 'c':
	         printAsCSV	= true;
	         break;

	      case 't':
	         tii_framedelay = atoi(optarg);
	         fprintf (stderr,
	                  "read option -t : tii framedelay %d\n",
	                  tii_framedelay);
	         break;

	      case 'a':
	         tii_alfa = atof (optarg);
	         fprintf (stderr,
	                  "read option -a : tii alfa %f\n",
	                  tii_alfa);
	         break;

	      case 'r':
	         tii_resetFrames = atoi (optarg);
	         fprintf (stderr,
	                  "read option -r : tii resetFrames %d\n",
	                  tii_resetFrames);
	         break;

	      case 'x':
	         useExTii = true;
	         fprintf (stderr,
	                  "read option -x : using %s Tii algorithm\n",
	                  (useExTii ? "extended" : "standard") );
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
#if defined(HAVE_RTLSDR)
	      case 'd':
	         deviceIndex	= atoi (optarg);
	         break;

	      case 's':
	         deviceSerial	= optarg;
	         break;

	      case 'O':
	         rtlOpts = optarg;
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
#elif	defined(HAVE_RTLSDR)
	   theDevice	= new rtlsdrHandler (frequency,
	                                     ppmCorrection,
	                                     theGain,
	                                     autogain,
	                                     (uint16_t)deviceIndex,
	                                     deviceSerial,
	                                     rtlOpts);
#elif	HAVE_WAVFILES
	   theDevice	= new wavFiles (fileName,
	                                fileOffset,
	                                device_eof_callback,
	                                nullptr);
#elif	HAVE_RAWFILES
	   theDevice	= new rawFiles (fileName,
	                                fileOffset,
	                                device_eof_callback,
	                                nullptr);
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
	   fprintf(stderr, "\n" FMT_DURATION "exiting main()\n" SINCE_START );
#endif
	   exit (32);
	}
//
//	and with a sound device we now can create a "backend"
	theRadio	= new dabProcessor (theDevice,
	                                    theMode,
	                                    syncsignalHandler,
	                                    systemData,
	                                    ensemblenameHandler,
	                                    programnameHandler,
	                                    fibQuality,
	                                    pcmHandler,
	                                    bytesOut_Handler,
	                                    dataOut_Handler,
	                                    programdataHandler,
	                                    mscQuality,
	                                    motdataHandler,	// MOT in PAD
	                                    NULL,
	                                    NULL,
	                                    NULL		// Ctx
	                                  );
	if (theRadio == NULL) {
	   fprintf (stderr, "sorry, no radio available, fatal\n");
	   nextOut	= timeOut;
	   printCollectedCallbackStat ("A: no radio");
#if PRINT_DURATION
	   fprintf (stderr, "\n" FMT_DURATION "exiting main()\n" SINCE_START );
#endif
	   exit (4);
	}

	if (useExTii)
	   theRadio -> setTII_handler (nullptr,
	                               tiiEx,
	                               tii_framedelay,
	                               tii_alfa,
	                               tii_resetFrames);
	else
	   theRadio -> setTII_handler (tii,
	                               nullptr,
	                               tii_framedelay,
	                               tii_alfa,
	                               tii_resetFrames);

	theDevice	-> setGain (theGain);
	if (autogain)
	   theDevice	-> set_autogain (autogain);
	theDevice	-> restartReader (frequency);
//
//	The device should be working right now

	timesyncSet.		store (false);
	ensembleRecognized.	store (false);
	fprintf (stderr,
	         "\n" FMT_DURATION "starting DAB processing ..\n" SINCE_START );
	theRadio -> start ();

	bool abortForSnr		= false;
	bool continueForFullEnsemble	= false;

#if PRINT_LOOPS
	fprintf (stderr,
	         "\n" FMT_DURATION "before while1: cont=%s, abort=%s, timeout=%d, waitTime=%d\n"
	         SINCE_START
	         , continueForFullEnsemble ? "true":"false"
	         , abortForSnr ? "true":"false"
	         , int (timeOut)
	         , int (waitingTime)
	         );
#endif

	printCollectedCallbackStat ("wait for timeSync ..");
	while ((timeOut += T_GRANULARITY) < waitingTime && !abortForSnr) {
	   if ((!ensembleRecognized. load () || continueForFullEnsemble) &&
	             timeOut > T_GRANULARITY )
	      sleepMillis (T_GRANULARITY);	// sleep (1);  // skip 1st sleep if possible
	   printCollectedCallbackStat ("wait for timeSync ..");
	   if (scanOnly &&  (numSnr >= 5) && (avgSnr < minSNRtoExit)) {
	      fprintf (stderr,
	               FMT_DURATION "abort because minSNR %d is not met. # is %ld avg %ld\n"
	                SINCE_START , int (minSNRtoExit), numSnr, avgSnr );
	      abortForSnr = true;
	      break;
	   }

	   if ((waitingTime >= waitingTimeInit) &&
	                        ensembleRecognized. load ()) {
	      const bool prevContinueForFullEnsemble = continueForFullEnsemble;
	      continueForFullEnsemble = true;
	      if (waitAfterEnsemble > 0) {
	         if (!prevContinueForFullEnsemble) {  // increase waitingTime only once
	            fprintf (stderr, "t=%d: abort later because already got ensemble data.\n", timeOut);
	            waitingTime = timeOut + waitAfterEnsemble;
	            fprintf (stderr,
	                     FMT_DURATION "waitAfterEnsemble = %d > 0  ==> waitingTime = timeOut + waitAfterEnsemble = %d + %d = %d\n"
	                           SINCE_START , waitAfterEnsemble
	                           , timeOut, waitAfterEnsemble, waitingTime);
	         }
	      }
	      else
	      if (waitAfterEnsemble == 0) {
	         fprintf (stderr,
	                  "t = %d: abort directly because already got ensemble data.\n",
	                  timeOut);
	         fprintf (stderr,
	                  FMT_DURATION "waitAfterEnsemble == 0  ==> break\n" SINCE_START);
	         break;
	      }
	   }

#if PRINT_LOOPS
	  fprintf (stderr,
	           FMT_DURATION "in while1: cont=%s, abort=%s, timeout=%d, waitTime=%d\n"
	           SINCE_START
	           , continueForFullEnsemble ? "true":"false"
	           , abortForSnr ? "true":"false"
	           , int (timeOut), int (waitingTime)
	          );
#endif
	}

#if PRINT_LOOPS
	fprintf (stderr,
	         "\n" FMT_DURATION "before while2: cont=%s, abort=%s, timeout=%d, waitTime=%d\n"
	          SINCE_START
	         , continueForFullEnsemble ? "true":"false"
	         , abortForSnr ? "true":"false"
	         , int (timeOut)
	         , int (waitingTime)
	       );
#endif

	while (continueForFullEnsemble &&
	       !abortForSnr &&  ((timeOut += T_GRANULARITY) < waitingTime)) {
	  sleepMillis (T_GRANULARITY);
	  printCollectedCallbackStat ("wait for full ensemble info..");
	  if ((scanOnly && numSnr >= 5) && (avgSnr < minSNRtoExit)) {
	    fprintf (stderr,
	             FMT_DURATION "abort because minSNR %d is not met. # is %ld avg %ld\n" SINCE_START 
		     , int(minSNRtoExit), numSnr, avgSnr );
	    abortForSnr = true;
	    break;
	  }
#if PRINT_LOOPS
	  fprintf(stderr, FMT_DURATION "in while2: cont=%s, abort=%s, timeout=%d, waitTime=%d\n"
	    SINCE_START
	    , continueForFullEnsemble ? "true":"false", abortForSnr ? "true":"false"
	    , int(timeOut), int(waitingTime) );
#endif
	}

	if (!timeSynced. load ()) {
	   fprintf(stderr, FMT_DURATION "There does not seem to be a DAB signal here\n" SINCE_START );
	   FAST_EXIT (22);
	   theDevice -> stopReader ();
	   theRadio -> stop ();
	   nextOut = timeOut;
	   printCollectedCallbackStat ("B: no DAB signal");
	   delete theRadio;
	   delete theDevice;
#if PRINT_DURATION
	   fprintf (stderr, "\n" FMT_DURATION "exiting main()\n" SINCE_START );
#endif
	   exit (22);
	}

#if PRINT_LOOPS
	fprintf (stderr,
	         "\n" FMT_DURATION "before while3: cont=%s, abort=%s, timeout=%d, waitTime=%d\n"
	          SINCE_START
	         , continueForFullEnsemble ? "true":"false"
	         , abortForSnr ? "true":"false"
	         , int(timeOut), int (waitingTime));
#endif
	while (!ensembleRecognized. load () &&
	       ((timeOut += T_GRANULARITY) < waitingTime) && !abortForSnr) {
	  sleepMillis (T_GRANULARITY);
	  printCollectedCallbackStat ("C: collecting ensembleData ..");
	  if (scanOnly &&  (numSnr >= 5) && (avgSnr < minSNRtoExit)) {
	    fprintf (stderr,
	             FMT_DURATION "abort because minSNR %d is not met. # is %ld avg %ld\n" SINCE_START 
	             , int (minSNRtoExit)
	             , numSnr, avgSnr);
	    abortForSnr = true;
	    break;
	  }

#if PRINT_LOOPS
	  fprintf (stderr, FMT_DURATION "in while3: cont=%s, abort=%s, timeout=%d, waitTime=%d\n"
	           SINCE_START 
	           , continueForFullEnsemble ? "true":"false"
	           , abortForSnr ? "true":"false"
	           , int (timeOut), int (waitingTime));
#endif
	}
	fprintf (stderr, "\n");
	if (abortForSnr)
	   exit (24);

	static	int count	= 10;
	
	while (!scanOnly && !ensembleRecognized. load () && (--count > 0))
	  sleep (1);

	if (!ensembleRecognized. load ()) {
	   fprintf (stderr, FMT_DURATION "no ensemble data found, fatal\n" SINCE_START );
	   FAST_EXIT (22);
	   theDevice -> stopReader ();
	   theRadio	-> stop ();
	   nextOut = timeOut;
	   printCollectedCallbackStat ("D: no ensembleData");
	   delete theRadio;
	   delete theDevice;
#if PRINT_DURATION
	   fprintf(stderr, "\n" FMT_DURATION "exiting main()\n" SINCE_START );
#endif
	   exit (22);
	}
//
//	we now know that the ensemble is recognized
	nextOut = timeOut;
	if (!printAsCSV)
	   printCollectedCallbackStat ("summmary for found ensemble", 1);

	if (!scanOnly) {
	   audiodata ad;
	   packetdata pd;
	   run. store (true);
	   std::cerr << "we try to start service " <<
	                                         programName << "\n";
	   theRadio -> dataforAudioService (programName. c_str (), &ad, 0);
	   theRadio -> dataforDataService (programName. c_str (), &pd, 0);
	   if (!(ad. defined || pd.defined)) {
	      std::cerr << "sorry  we cannot handle service " <<
	      programName << "\n";
	      run. store (false);
	   }

	   if (run. load ()) {
	      theRadio -> reset_msc ();
	      if (ad. defined)
	         theRadio -> set_audioChannel (&ad);
	      else
	         theRadio -> set_dataChannel  (&pd);
	      while (run. load ()) {
	         timeOut += T_GRANULARITY;
	         sleepMillis (T_GRANULARITY);
	         printCollectedCallbackStat ("E: loading ..");
	      }
	      
#if LOG_EX_TII_SPECTRUM
	      if (useExTii)
	         writeTiiExBuffer();
#endif

	   }
	}
	else {	// scan only
	   uint64_t secsEpoch	= msecs_progStart / 1000;
	   bool gotECC		= false;
	   bool gotInterTabId	= false;
	   const uint8_t eccCode =
	              theRadio -> getECC (&gotECC);
	   const uint8_t interTabId =
	               theRadio -> getInterTabId (&gotInterTabId);
	   const std::string comma = ",";
	
#if LOG_EX_TII_SPECTRUM
	   if (useExTii)
	      writeTiiExBuffer();
#endif
	
	   std::string ensembleCols;
	   std::string ensembleComm;

	   std::stringstream sidStrStream;
	   sidStrStream << std::hex << ensembleIdentifier;

	   ensembleCols = comma + "0x" + sidStrStream.str ();
	   ensembleComm = comma + "SID";
	   if (ensembleRecognized. load ()) {
	      ensembleCols += comma + prepCsvStr (ensembleName);
	   }
	   else {
	      ensembleCols += comma + prepCsvStr ("unknown ensemble");
	   }
	   ensembleComm = comma + "ensembleName";

	   int mostTii = -1;
	   int numMostTii = 0;
	   for (auto const& x : tiiMap) {
	      if (x. second > numMostTii && x. first > 0) {
	         numMostTii 	= x.second;
	         mostTii 	= x.first;
	      }
	   }

	   if (printAsCSV) {
	      std::string outLine = std::to_string (secsEpoch);
	      std::string outComm = std::to_string (secsEpoch);
	      outLine += comma + "CSV_ENSEMBLE";
	      outComm += comma + "#CSV_ENSEMBLE";
	      outLine += comma + prepCsvStr (theChannel);
	      outComm += comma + "channel";
	      outLine += ensembleCols;
	      outComm += ensembleComm;

	      outLine += comma + prepCsvStr ("snr");
	      outComm += comma + "snr";
	      outLine += comma + std::to_string (int (stat_minSnr));
	      outComm += comma + "min_snr";
	      outLine += comma + std::to_string( int (stat_maxSnr) );
	      outComm += comma + "max_snr";
	      outLine += comma + std::to_string( int (numSnr) );
	      outComm += comma + "num_snr";
	      outLine += comma + std::to_string( int (avgSnr) );
	      outComm += comma + "avg_snr";

	      outLine += comma + prepCsvStr( "fic" );
	      outComm += comma + "fic";
	      outLine += comma + std::to_string( int (stat_minFic) );
	      outComm += comma + "min_fic";
	      outLine += comma + std::to_string( int (stat_maxFic) );
	      outComm += comma + "max_fic";
	      outLine += comma + std::to_string( int (numFic) );
	      outComm += comma + "num_fic";
	      outLine += comma + std::to_string( int (avgFic) );
	      outComm += comma + "avg_fic";
	      
	      outLine += comma + prepCsvStr( "tii" );
	      outComm += comma + "tii";
	      if (useExTii) {
	         std::vector<ExTiiInfo> tiiExVec;
	         tiiExVec.reserve( tiiExMap.size() ); 
	         for (auto const& x : tiiExMap) {
	             tiiExVec.push_back(x.second);
	         }
	         std::sort(tiiExVec.begin(), tiiExVec.end());
	         for (auto const& x : tiiExVec) {
	            outLine += comma + std::to_string( x.tii );
	            outComm += comma + "tii_id";
	            outLine += comma + std::to_string( x.numOccurences );
	            outComm += comma + "num";
	            outLine += comma + std::to_string( int( ( x.maxAvgSNR < 0.0F ? -0.5F : 0.5F ) + 10.0F * x.maxAvgSNR ) );
	            outComm += comma + "max(avg_snr)";
	            outLine += comma + std::to_string( int( ( x.maxMinSNR < 0.0F ? -0.5F : 0.5F ) + 10.0F * x.maxMinSNR ) );
	            outComm += comma + "max(min_snr)";
	            outLine += comma + std::to_string( int( ( x.maxNxtSNR < 0.0F ? -0.5F : 0.5F ) + 10.0F * x.maxNxtSNR ) );
	            outComm += comma + "max(next_snr)";
	            outLine += comma;
	            outComm += comma;
	         }
	      } else {
	         outLine += comma + std::to_string( int (numMostTii) );
	         outComm += comma + "tii_id";
	         outLine += comma + std::to_string( int (numAllTii) );
	         outComm += comma + "num_all";
	         outLine += comma + std::to_string( int (mostTii) );
	         outComm += comma + "num_id";
	      }
	      fprintf(infoStrm, "%s\n", outComm.c_str() );
	      fprintf(infoStrm, "%s\n", outLine.c_str() );
	   }

	   int16_t mainId, subId, TD;
	   float gps_latitude, gps_longitude;
	   bool gps_success = false;
	   theRadio -> get_coordinates (-1, -1,
	                                &gps_success,
	                                &mainId, &subId, NULL);

	   if (gps_success) {
	      for (subId = 1; subId <= 23; ++subId ) {
	         theRadio -> get_coordinates (mainId, subId,
	                                      &gps_success,
	                                      nullptr, nullptr, &TD);
	         if (gps_success) {
	             if (!printAsCSV) {
	                fprintf (infoStrm,
	                         "\ttransmitter gps coordinate (latitude / longitude) for\ttii %04d\t=%f / %f\tTD=%d us\n",
	                         mainId * 100 + subId,
	                         gps_latitude,
	                         gps_longitude, int(TD) );
	             } else {
	                std::string outLine = std::to_string (secsEpoch);
	                outLine += comma + "CSV_GPSCOOR";
	                outLine += comma + prepCsvStr( theChannel );
	                outLine += ensembleCols;

	                outLine += comma +
	                          std::to_string( mainId * 100 + subId);
	                outLine += comma + std::to_string (gps_latitude);
	                outLine += comma + std::to_string (gps_longitude);
	                outLine += comma + std::to_string (int (TD));
	                fprintf (infoStrm, "%s\n", outLine.c_str ());
	             }
	         }
	      }	// end for
	   }	// end if (gps_success)

	   std::string outAudioBeg = std::to_string (secsEpoch);
	   outAudioBeg += comma + "CSV_AUDIO";
	   outAudioBeg += comma + prepCsvStr( theChannel );
	   outAudioBeg += ensembleCols;

	   std::string outPacketBeg = std::to_string( secsEpoch );
	   outPacketBeg += comma + "CSV_PACKET";
	   outPacketBeg += comma + prepCsvStr( theChannel );
	   outPacketBeg += ensembleCols;

	   for (auto & it : globals.channels ) {
	      serviceIdentifier = it.first;
	      std::string serviceName = it. second -> programName;
	      uint8_t serviceKind =
	              theRadio -> kindofService (serviceName. c_str ());
	      if ((serviceKind == AUDIO_SERVICE) ||
	         (serviceKind ==  PACKET_SERVICE)) {
	         if (!printAsCSV) { fprintf (infoStrm,
	                      "\n" FMT_DURATION "checked program '%s' with SId %X\n"
	                   SINCE_START , it. second -> programName.c_str(), serviceIdentifier);
	         }

	         for (int i = 0; i < 5; i ++) {
	            audiodata ad;
	            packetdata pd;
	            theRadio -> dataforAudioService (
	                                 it. second -> programName. c_str (),
	                                 &ad,
	                                 i);
	
	            if (ad. defined) {
	               uint8_t countryId =
	                        (serviceIdentifier >> 12) & 0xF;  // audio
	               if (!printAsCSV) {
	                   fprintf (infoStrm, "\taudioData:\n");
	                   fprintf (infoStrm, "\t\tsubchId\t\t= %d\n",
	                                                 int (ad. subchId));
	                   fprintf (infoStrm, "\t\tstartAddr\t= %d\n",
	                                                 int (ad. startAddr));
	                   fprintf (infoStrm, "\t\tshortForm\t= %s\n",
	                                       ad. shortForm ? "true":"false");
	                   fprintf (infoStrm, "\t\tprotLevel\t= %d: '%s'\n",
	                                       int (ad. protLevel),
	                                       getProtectionLevel (ad. shortForm,
	                                                           ad. protLevel));
	                   fprintf (infoStrm, "\t\tcodeRate\t= %d: '%s'\n",
	                                   int (ad. protLevel),
	                                   getCodeRate (ad. shortForm,
	                                                ad. protLevel));
	                   fprintf (infoStrm, "\t\tlength\t\t= %d\n",
	                                   int (ad. length));
	                   fprintf (infoStrm, "\t\tbitRate\t\t= %d\n",
	                                   int (ad. bitRate));
	                   fprintf (infoStrm,
	                            "\t\tASCTy\t\t= %d: '%s'\n",
	                                   int (ad. ASCTy),
	                                   getASCTy (ad. ASCTy));
	                   if (gotECC)
	                      fprintf (infoStrm,
	                               "\t\tcountry\tECC %X, Id %X: '%s'\n", 
	                               int (eccCode), int(countryId),
	                               getCountry (eccCode, countryId));
	                   fprintf (infoStrm,
	                            "\t\tlanguage\t= %d: '%s'\n",
	                            int (ad. language),
	                            getLanguage (ad. language));
	                   fprintf (infoStrm,
	                            "\t\tprogramType\t= %d: '%s'\n",
	                            int (ad. programType),
	                            getProgramType (gotInterTabId,
	                                            interTabId,
	                                            ad. programType) );
	                                            
	               } else {
	                   std::string outLine = outAudioBeg;
	                   std::stringstream sidStrStream;
	                   sidStrStream << std::hex << serviceIdentifier;
	                   outLine += comma + "0x" + sidStrStream.str();
	                   outLine += comma + prepCsvStr( it. second -> programName.c_str() );
	                   outLine += comma + std::to_string( i );
	                   outLine += comma + std::to_string( countryId );
	                   outLine += comma + prepCsvStr( getProtectionLevel(ad. shortForm, ad. protLevel) );
	                   outLine += comma + std::to_string( int (ad. protLevel) );
	                   outLine += comma + prepCsvStr( getCodeRate(ad. shortForm, ad. protLevel) );
	                   outLine += comma + std::to_string( int (ad. bitRate) );
	                   outLine += comma + std::to_string( int (ad. ASCTy) );
	                   outLine += comma + prepCsvStr( getASCTy(ad. ASCTy) );
	                   if (gotECC) {
	                       std::stringstream eccStrStream;
	                       std::stringstream countryStrStream;
	                       eccStrStream << std::hex << int (eccCode);
	                       countryStrStream << std::hex << int(countryId);
	                       outLine += comma + "0x" + eccStrStream.str();
	                       outLine += comma + "0x" + countryStrStream.str();
	                       const char * countryStr = getCountry(eccCode, countryId);
	                       outLine += comma + ( countryStr ? prepCsvStr( countryStr ) : prepCsvStr( "unknown country" ) );
	                   } else {
	                       outLine += comma + comma + comma;
	                   }
	                   outLine += comma + std::to_string( int (ad. language) );
	                   outLine += comma + prepCsvStr( getLanguage (ad. language) );
	                   outLine += comma + std::to_string( int (ad. programType) );
	                   outLine += comma + prepCsvStr( getProgramType(gotInterTabId, interTabId, ad. programType) );

	                   fprintf(infoStrm, "%s\n", outLine.c_str() );
	               }
	            }
	            else {
	               theRadio -> dataforDataService (
	                                   it. second -> programName. c_str (),
	                                   &pd,
	                                   i);
	      
	               if (pd. defined) {
	                  uint8_t countryId =
	                            (serviceIdentifier >> (5 * 4)) & 0xF;
	                  if (!printAsCSV) {
	                      fprintf (infoStrm, "\tpacket:\n");
	                      fprintf (infoStrm,
	                                  "\t\tsubchId\t\t= %d\n",
	                                          int (pd. subchId));
	                      fprintf (infoStrm,
	                                  "\t\tstartAddr\t= %d\n",
	                                          int (pd. startAddr));
	                      fprintf (infoStrm,
	                                  "\t\tshortForm\t= %s\n",
	                                          pd. shortForm ? "true":"false");
	                      fprintf (infoStrm,
	                                  "\t\tprotLevel\t= %d: '%s'\n",
	                                          int (pd. protLevel),
	                                          getProtectionLevel (pd. shortForm,
	                                                              pd. protLevel));
	                      fprintf (infoStrm,
	                                  "\t\tcodeRate\t= %d: '%s'\n",
	                                          int (pd. protLevel),
	                                          getCodeRate (pd. shortForm,
	                                                       pd. protLevel));
	                      fprintf (infoStrm,
	                                  "\t\tDSCTy\t\t= %d: '%s'\n",
	                                         int (pd. DSCTy),
	                                         getDSCTy (pd. DSCTy));
	                      fprintf (infoStrm,
	                                  "\t\tlength\t\t= %d\n", int (pd. length));
	                      fprintf (infoStrm,
	                                  "\t\tbitRate\t\t= %d\n", int (pd. bitRate));
	                      fprintf (infoStrm,
	                                  "\t\tFEC_scheme\t= %d: '%s'\n",
	                                       int (pd. FEC_scheme),
	                                       getFECscheme (pd. FEC_scheme));
	                      fprintf (infoStrm,
	                                  "\t\tDGflag\t= %d\n", int (pd. DGflag));
	                      fprintf (infoStrm,
	                                  "\t\tpacketAddress\t= %d\n",
	                                          int (pd. packetAddress));
	                      if (gotECC)
	                         fprintf (infoStrm,
	                                     "\t\tcountry\tECC %X, Id %X: '%s'\n",
	                                           int (eccCode),
	                                           int (countryId),
	                                           getCountry (eccCode, countryId));
	                      fprintf (infoStrm,
	                               "\t\tappType\t\t= %d: '%s'\n",
	                                    int (pd. appType),
	                                    getUserApplicationType (pd. appType));
	                      fprintf (infoStrm,
	                               "\t\tis_madePublic\t=%s\n",
	                                    pd. is_madePublic ? "true":"false");
	                  } else {
	                   std::string outLine = outPacketBeg;
	                   std::stringstream sidStrStream;
	                   sidStrStream << std::hex << serviceIdentifier;
	                   outLine += comma + "0x" + sidStrStream.str();
	                   outLine += comma + prepCsvStr( it. second -> programName.c_str() );
	                   outLine += comma + std::to_string( i );
	                   outLine += comma + std::to_string( int (pd. protLevel) );
	                   outLine += comma + prepCsvStr( getProtectionLevel (pd. shortForm, pd. protLevel) );
	                   outLine += comma + prepCsvStr( getCodeRate(pd. shortForm, pd. protLevel) );
	                   outLine += comma + std::to_string( int (pd. DSCTy) );
	                   outLine += comma + prepCsvStr( getDSCTy (pd. DSCTy) );
	                   outLine += comma + std::to_string( int (pd. bitRate) );
	                   outLine += comma + std::to_string( int (pd. FEC_scheme) );
	                   outLine += comma + prepCsvStr( getFECscheme (pd. FEC_scheme) );
	                   outLine += comma + std::to_string( int (pd. DGflag) );
	                   if (gotECC) {
	                       std::stringstream eccStrStream;
	                       std::stringstream countryStrStream;
	                       eccStrStream << std::hex << int (eccCode);
	                       countryStrStream << std::hex << int(countryId);
	                       outLine += comma + "0x" + eccStrStream.str();
	                       outLine += comma + "0x" + countryStrStream.str();
	                       const char * countryStr = getCountry(eccCode, countryId);
	                       outLine += comma + ( countryStr ? prepCsvStr( countryStr ) : prepCsvStr( "unknown country" ) );
	                   } else {
	                       outLine += comma + comma + comma;
	                   }
	                   outLine += comma + std::to_string( int (pd. appType) );
	                   outLine += comma + prepCsvStr( getUserApplicationType (pd. appType) );

	                   fprintf(infoStrm, "%s\n", outLine.c_str() );
	                  }
	               }
	            }
	         }
	      }
	   }

	   nextOut = timeOut;
	   printCollectedCallbackStat("D: quit without loading");
	}

#if PRINT_DURATION
	fprintf(stderr, "\n" FMT_DURATION "at dabStop()\n" SINCE_START );
#endif

	FAST_EXIT( 123 );
	theRadio	-> stop ();
	theDevice	-> stopReader ();
	delete	theRadio;
	delete theDevice;

#if PRINT_DURATION
	fprintf(stderr, "\n" FMT_DURATION "end of main()\n" SINCE_START );
#endif
}

void    printOptions (void) {
	fprintf (stderr,
"	dab-cmdline options are\n\
	-W number   amount of time to look for a time sync in %s\n\
	-A number   amount of time to look for an ensemble in %s\n\
	-E minSNR   activates scan mode: if set, quit after loading scan data\n\
	            also quit, if SNR is below minSNR\n\
	-t number   determine tii every number frames. default is 10\n\
	-a alfa     update tii spectral power with factor alfa in 0 to 1. default: 0.9\n\
	-r number   reset tii spectral power every number frames. default: 10\n\
	            tii statistics is output in scan mode\n\
	-c          activates CSV output mode\n\
	-M Mode     Mode is 1, 2 or 4. Default is Mode 1\n\
	-B Band     Band is either L_BAND or BAND_III (default)\n\
	-P name     program to be selected in the ensemble\n\
	-p ppmCorr  ppm correction\n\
	-O options  set device specific option string - seperated with ':'\n\
	            currently only options for RTLSDR available:\n\
	            f=<freqHz>:bw=<bw_in_kHz>:agc=<tuner_gain_mode>:gain=<tenth_dB>\n\
	            dagc=<rtl_agc>:ds=<direct_sampling_mode>:T=<bias_tee>\n"
#if defined(HAVE_RTLSDR)
"	-d index    set RTLSDR device index\n\
	-s serial   set RTLSDR device serial\n"
#endif
#if	defined (HAVE_WAVFILES) || defined (HAVE_RAWFILES)
"	-F filename in case the input is from file\n"
"	-o offset   offset in seconds from where to start file playback\n"
"	-R          deactivates repetition of file playback\n"

#else
"	-C channel  channel to be used\n\
	-G Gain     gain for device (range 1 .. 100)\n\
	-Q          if set, set autogain for device true\n"
#ifdef	HAVE_RTL_TCP
"        -H hostname  hostname for rtl_tcp\n\
	-I port      port number to rtl_tcp\n"
#endif
#endif
"	-S hexnumber use hexnumber to identify program\n\n", T_UNITS, T_UNITS );
}

