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
#include	"audiosink.h"
#include	"config.h"
#include	"radiodata.h"
#include	"includes/support/band-handler.h"
#include	"tcp-writer.h"
#ifdef	HAVE_SDRPLAY
#include	"sdrplay-handler.h"
#elif	HAVE_AIRSPY
#include	"airspy-handler.h"
#elif	HAVE_RTLSDR
#include	"rtlsdr-handler.h"
#elif	HAVE_WAVFILES
#include	"wavfiles.h"
#elif	HAVE_RTL_TCP
#include	"rtl_tcp-client.h"
#endif

using std::cerr;
using std::endl;

void	handleRequest	(void);
void    listener	(void);

//	we deal with some callbacks, so we have some data that needs
//	to be accessed from global contexts
static
void	*theRadio	= nullptr;

static
tcpWriter	*theWriter;

static
RingBuffer<uint8_t> *buffer;

static
bandHandler	*theBandHandler;

static
std::atomic<bool>timeSynced;

static
std::atomic<bool>timesyncSet;

static
std::atomic<bool>ensembleRecognized;

static
audioBase	*soundOut	= NULL;

static
std::atomic<bool>running;
void	readSettings (radioData *rd);

static void sighandler (int signum) {
        fprintf (stderr, "Signal caught, terminating!\n");
	running. store (false);
}

static
void	syncsignalHandler (bool b, void *userData) {
	timeSynced. store (b);
	timesyncSet. store (true);
	if (!b)
	   theWriter	-> sendData (Q_TEXT_MESSAGE,
	                             std::string ("no dab signal yet"));
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
	theWriter	-> sendData (Q_ENSEMBLE, name);
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
	theWriter	-> sendData (Q_SERVICE_NAME, s);
}

static
void	programdataHandler (audiodata *d, void *ctx) {
char storage [256];

	(void)ctx;
	sprintf (storage, "startaddress= %d", d -> startAddr);
	theWriter	-> sendData (Q_PROGRAM_DATA, std::string (storage));
	sprintf (storage, "length      = %d",     d -> length);
	theWriter	-> sendData (Q_PROGRAM_DATA, std::string (storage));
	sprintf (storage, "subChId     = %d",    d -> subchId);
	theWriter	-> sendData (Q_PROGRAM_DATA, std::string (storage));
	sprintf (storage, "protection  = %d",   d -> protLevel);
	theWriter	-> sendData (Q_PROGRAM_DATA, std::string (storage));
	sprintf (storage, "bitrate     = %d",    d -> bitRate);
	theWriter	-> sendData (Q_PROGRAM_DATA, std::string (storage));
}

//
//	The function is called from within the library with
//	a string, the so-called dynamic label
static
std::string localString;
static
void	dataOut_Handler (std::string dynamicLabel, void *ctx) {
	localString = dynamicLabel;
	(void)ctx;
	theWriter	-> sendData (Q_TEXT_MESSAGE, localString);
}
//
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

static
radioData my_radioData;
static
deviceHandler	*theDevice;

#include	"protocol.h"

int	main (int argc, char **argv) {
struct sigaction sigact;
bool	err;

	theBandHandler			= new bandHandler ();
	buffer				= new RingBuffer<uint8_t> (1024);
//	The defaults
	my_radioData. theMode		= 1;
	my_radioData. theChannel	= "11C";
	my_radioData. theBand		= BAND_III;
	my_radioData. ppmCorrection	= 0;
	my_radioData. theGain	= 45;	// scale = 0 .. 100
	my_radioData. soundChannel	= "default";
	my_radioData. latency		= 10;
	my_radioData. waitingTime	= 10;
	my_radioData. autogain		= false;
	my_radioData. hostname		= "127.0.0.1";		// default
	my_radioData. basePort		= 1234;		// default
	my_radioData. serverPort	= BASE_PORT;

	readSettings (&my_radioData);

	timeSynced.	store (false);
	timesyncSet.	store (false);
	running.	store (false);

	
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;

	int32_t frequency	= 220000000;	// default

	try {
#ifdef	HAVE_SDRPLAY
	   theDevice	= new sdrplayHandler (frequency,
	                                      my_radioData. ppmCorrection,
	                                      my_radioData. theGain,
	                                      3,	// lnaState
	                                      my_radioData. autogain,
	                                      0,
	                                      0);
#elif	HAVE_AIRSPY
	   theDevice	= new airspyHandler (frequency,
	                                     my_radioData. ppmCorrection,
	                                     my_radioData. theGain);
#elif	HAVE_RTLSDR
	   theDevice	= new rtlsdrHandler (frequency,
	                                     my_radioData. ppmCorrection,
	                                     my_radioData. theGain,
	                                     my_radioData. autogain);
#elif	HAVE_WAVFILES
	   theDevice	= new wavFiles (fileName);
#elif	HAVE_RTL_TCP
	   theDevice	= new rtl_tcp_client (hostname,
	                                      basePort,
	                                      frequency,
	                                      my_radioData. theGain,
	                                      my_radioData. autogain,
	                                      my_radioData. ppmCorrection);
#endif

	}
	catch (int e) {
	   fprintf (stderr, "allocating device failed (%d), fatal\n", e);
	   exit (32);
	}
//
	soundOut	= new audioSink	(my_radioData. latency,
	                                 my_radioData. soundChannel, &err);
	if (err) {
	   fprintf (stderr, "no valid sound channel, fatal\n");
	   exit (33);
	}
//
//	and with a sound device we can create a "backend"
	theRadio	= dabInit (theDevice,
	                           my_radioData. theMode,
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
	                           nullptr,	// no mot slides
	                           nullptr,	// no spectrum shown
	                           nullptr,	// no constellations
	                           nullptr	// ctx
	                          );
	if (theRadio == nullptr) {
	   fprintf (stderr, "sorry, no radio available, fatal\n");
	   exit (4);
	}

	theDevice	-> setGain (my_radioData. theGain);
	if (my_radioData. autogain)
	   theDevice	-> set_autogain (my_radioData. autogain);

	running. store (true);
	std::thread port_listener	= std::thread (listener);
//
//	the writer is an external task
	theWriter		= new tcpWriter (my_radioData. serverPort + 1);
	
	while (running. load ())
	   sleep (1);
	theDevice	-> stopReader ();
	dabStop (theRadio);
	delete	theWriter;
	dabExit	(theRadio);
	delete	theDevice;	
	delete	soundOut;
}

void	readSettings (radioData *rd) {
config *my_config;
std::string value;

	try {
	   my_config = new config ("home/jan/.radioconfig.conf");
	} catch (...) {
	   return;		// no config file
	}

	value	= my_config -> get_value ("ALL", "Mode");
	if (value != std::string (""))
	   rd	-> theMode	= stoi (value);
	value	= my_config	-> get_value ("ALL", "Band");
	if (value != std::string (""))
	   rd	-> theBand	= stoi (value);
	value	= my_config	-> get_value ("ALL", "ServerPort");
	if (value != std::string (""))
	   rd	-> serverPort	= stoi (value);
}

//	The controller "listens" to the port as given and
//	executes the commands for setting the radio

#define	BUF_SIZE	1024
void	listener (void) {
	struct sockaddr_in server;
	struct sockaddr_in client;
	int	socket_desc;
	int	client_sock;
	int	c;

	socket_desc = socket (AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
	   fprintf (stderr, "Could not create socket");
	   return;
	}
	fprintf (stderr, "Socket created");
//	Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons (my_radioData. serverPort);
	
//	Bind
	if (bind (socket_desc,
	          (struct sockaddr *)&server , sizeof(server)) < 0) {
//	print the error message
	   perror ("bind failed. Error");
	   return;
	}

	fprintf (stderr, "now accepting connections on port %d\n",
	                            my_radioData. serverPort);
	listen (socket_desc , 3);

	while (running. load ()) {
//	Accept a new connection and return back the socket desciptor 
	   c = sizeof (struct sockaddr_in);
     
//	accept connection from an incoming client
	   client_sock = accept (socket_desc, 
	                         (struct sockaddr *)&client,
	                         (socklen_t*)&c);
	   if (client_sock < 0) {
	      perror("accept failed");
	      return;
	   }
	   fprintf (stderr, "Connection accepted");
	   try {
	      uint8_t	localBuffer [PACKET_SIZE];
	      while (running. load ()) {
	         int amount = recv (client_sock,
	                            localBuffer, PACKET_SIZE, 0);
	         if (amount <= 0) {
	            throw (22);
	         }
	         else {
	            fprintf (stderr, "packet gelezen size %d\n", amount);
	            buffer -> putDataIntoBuffer (localBuffer, PACKET_SIZE);
	            handleRequest ();
	         }
	      }
	   }
	   catch (int e) {
	      fprintf (stderr, "disconnected\n");
	      dabStop (theRadio);
	   }
	}
	close (socket_desc);	
}

void	handleRequest (void) {
int32_t	frequency;

	fprintf (stderr, "handling requests\n");
	fprintf (stderr, "bufferfill %d\n",
	                buffer -> GetRingBufferReadAvailable ());
	while (buffer -> GetRingBufferReadAvailable () >= PACKET_SIZE) {
	   uint8_t lbuf [PACKET_SIZE];
	   buffer	-> getDataFromBuffer (lbuf, PACKET_SIZE);
	   switch (lbuf [2]) {
	      case Q_QUIT:
	         fprintf (stderr, "quit request\n");
	         running. store (false);
	         throw (33);
	         break;

	      case Q_GAIN:
	         fprintf (stderr, "gain string is %s\n", &(lbuf [3]));
	         my_radioData. theGain =
	                stoi (std::string ((char *)(&(lbuf [3]))));
	         theDevice	-> setGain (my_radioData. theGain);
	         break;

	      case Q_SERVICE:
	         my_radioData. serviceName =
	                          std::string ((char *)(&(lbuf [3])));
	         fprintf (stderr, "service request for %s\n",
	                              my_radioData. serviceName. c_str ());
	         
	         if (is_audioService (theRadio,
	                              my_radioData. serviceName. c_str ())) {
	            audiodata ad;
	            dataforAudioService (theRadio,
	                                 my_radioData. serviceName. c_str (),
	                                 &ad, 0);
	            if (!ad. defined) {
	               std::cerr << "sorry  we cannot handle service " <<
                                            my_radioData. serviceName << "\n";
	               running. store (false);
	            }
	            else {
	               dabReset_msc (theRadio);
	               set_audioChannel (theRadio, &ad);
	            }
	         }
	         break;

	      case Q_CHANNEL:
	         fprintf (stderr, "channel request\n");
	         dabStop (theRadio);
	         theDevice	-> stopReader ();
	         fprintf (stderr, "radio and device stopped\n");
	         my_radioData. theChannel = std::string ((char *)(&(lbuf [3])));
	         fprintf (stderr, "selecting channel %s\n", 
	                              (char *)(&(buffer [1])));
	         programNames. resize (0);
	         programSIds . resize (0);
	         frequency =
	               theBandHandler -> Frequency (my_radioData. theBand,
	                                            my_radioData. theChannel);
	         dabStartProcessing (theRadio);
	         theDevice	-> restartReader (frequency);
	         break;

	      default:
	         break;
	   }
	}
}

