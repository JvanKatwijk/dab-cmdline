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
#include        <stdio.h>
#include        <stdbool.h>
#include        <stdint.h>
#include        <sys/socket.h>
#include        <bluetooth/bluetooth.h>
#include        <bluetooth/rfcomm.h>
#include        <bluetooth/sdp.h>
#include        <bluetooth/sdp_lib.h>
#include	<signal.h>
#include	<getopt.h>
#include        <cstdio>
#include        <iostream>
#include	<complex>
#include	<vector>
#include        <pthread.h>
#include	"dab-api.h"
#include	"audiosink.h"
#include	"config.h"
#include	"radiodata.h"
#include	"band-handler.h"
#include	"protocol.h"

#ifdef	HAVE_SDRPLAY
#include	"sdrplay-handler.h"
#elif	HAVE_AIRSPY
#include	"airspy-handler.h"
#elif	HAVE_RTLSDR
#include	"rtlsdr-handler.h"
#endif

using std::cerr;
using std::endl;


//	we deal with some callbacks, so we have some data that needs
//	to be accessed from global contexts
static
void	*theRadio	= nullptr;

static
bandHandler	*theBandHandler;

static
int	client;

static
void    handleRequest (char *buf, int bufLen);
static
sdp_session_t *register_service (void);

//
//      This function is called from - most likely -
//      different threads from within the library
static
void    messageWriter (uint8_t code, std::string theText) {
int     len     = theText. length ();
int     i;
int     message [len + 3 + 1];
        message [0]     = (char)code;
        message [1]     = (len >> 8) & 0xFF;
        message [2]     = len & 0xFF;
        for (i = 0; i < len; i ++)
           message [3 + i] = theText [i];
        message [len + 3] = (uint8_t)0;
//	locker. lock ();
        write (client, message, len + 3 + 1);
//	locker. unlock ();
}

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
void		handleSettings (radioData *rd);
deviceHandler	*getDevice (int, radioData *rd);

static void sighandler (int signum) {
        fprintf (stderr, "Signal caught, terminating!\n");
	running. store (false);
}

static
void	syncsignalHandler (bool b, void *userData) {
	timeSynced. store (b);
	timesyncSet. store (true);
	if (!b)
	   messageWriter (Q_TEXT_MESSAGE,
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
	messageWriter (Q_ENSEMBLE, name);
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
	messageWriter (Q_SERVICE_NAME, s);
}

static
void	programdataHandler (audiodata *d, void *ctx) {
char storage [256];

	(void)ctx;
	sprintf (storage, "startaddress= %d", d -> startAddr);
	messageWriter (Q_PROGRAM_DATA, std::string (storage));
	sprintf (storage, "length      = %d",     d -> length);
	messageWriter (Q_PROGRAM_DATA, std::string (storage));
	sprintf (storage, "subChId     = %d",    d -> subchId);
	messageWriter (Q_PROGRAM_DATA, std::string (storage));
	sprintf (storage, "protection  = %d",   d -> protLevel);
	messageWriter (Q_PROGRAM_DATA, std::string (storage));
	sprintf (storage, "bitrate     = %d",    d -> bitRate);
	messageWriter (Q_PROGRAM_DATA, std::string (storage));
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
	messageWriter (Q_TEXT_MESSAGE, localString);
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
struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
char buf [1024] = { 0 };
int s;
socklen_t opt = sizeof (rem_addr);
bdaddr_t tmp	= (bdaddr_t) {{0, 0, 0, 0, 0, 0}};
	theBandHandler			= new bandHandler ();
	handleSettings (&my_radioData);

        register_service ();
//      allocate socket
        s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

//      bind socket to port 1 of the first available
//      local bluetooth adapter
        loc_addr.rc_family = AF_BLUETOOTH;
	
        loc_addr.rc_bdaddr = tmp;
//	loc_addr.rc_bdaddr = *BDADDR_ANY;
        loc_addr.rc_channel = (uint8_t) 1;
        bind (s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
	
	timeSynced.	store (false);
	timesyncSet.	store (false);
	running.	store (false);

	
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;

	int32_t frequency	= 220000000;	// default

	try {
	   theDevice	= getDevice (frequency, &my_radioData);
	}
	catch (int e) {
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
	theDevice	-> setVFOFrequency (frequency);

	running. store (true);
	
//
	while (1) {
//      put socket into listening mode
           fprintf (stderr, "server is listening\n");
           listen (s, 1);
//      accept one connection
           client = accept (s, (struct sockaddr *)&rem_addr, &opt);
//
//      reset the state of the receiver here
           ba2str (&rem_addr. rc_bdaddr, buf);
           fprintf (stderr, "accepted connection from %s\n", buf);
           memset (buf, 0, sizeof (buf));

           while (true) {
              int bytes_read;
              char buf [1024];
              bytes_read = read (client, buf, sizeof (buf));
              if (bytes_read > 0)
                 handleRequest (buf, bytes_read);
              else {
                 fprintf (stderr, "read gives back %d\n", bytes_read);
                 break;
	      }
	   }
	}

	theDevice	-> stopReader ();
	dabStop (theRadio);
	dabExit	(theRadio);
	delete	theDevice;	
	delete	soundOut;
	close (client);
	close (s);
}

void	handleSettings (radioData *rd) {
config *my_config;
std::string value;

//	The defaults
	rd	-> theMode		= 1;
	rd	-> theChannel		= "11C";
	rd	-> theBand		= BAND_III;
	rd	-> ppmCorrection	= 0;
	rd	-> theGain		= 35;	// scale = 0 .. 100
	rd	-> soundChannel		= "default";
	rd	-> latency		= 10;
	rd	-> waitingTime		= 10;
	rd	-> autogain		= false;

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
}

void	handleRequest (char *lbuf, int bufLen) {
	switch (lbuf [0]) {
	   case Q_QUIT:
	      fprintf (stderr, "quit request\n");
	      break;

	   case Q_GAIN:
	      fprintf (stderr, "gain string is %s\n", &(lbuf [3]));
	      my_radioData. theGain =
	                stoi (std::string ((char *)(&(lbuf [3]))));
	      theDevice      -> setGain (my_radioData. theGain);
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
	      theDevice      -> stopReader ();
	      fprintf (stderr, "radio and device stopped\n");
	      my_radioData. theChannel = std::string ((char *)(&(lbuf [3])));
	      fprintf (stderr, "selecting channel %s\n",
	                                      (char *)(&(lbuf [3])));
	      {  int frequency =
	                 theBandHandler -> Frequency (my_radioData. theBand,
	                                              my_radioData. theChannel);
	         theDevice   -> setVFOFrequency (frequency);
	      }
	      dabStartProcessing (theRadio);
	      programNames. resize (0);
	      programSIds . resize (0);
	      theDevice      -> restartReader ();
	      break;

	   default:
	      fprintf (stderr, "Message not understood\n");
	      break;
	}
}

deviceHandler	*getDevice (int frequency, radioData *rd) {
	try {
#ifdef	HAVE_SDRPLAY
	   return new sdrplayHandler (frequency,
	                              rd  -> ppmCorrection,
	                              rd  -> theGain,
	                              rd  -> autogain,
	                              0,
	                              0);
#elif	HAVE_AIRSPY
	   return new airspyHandler (frequency,
	                             rd  -> ppmCorrection,
	                             rd  -> theGain);
#elif	HAVE_RTLSDR
	   return new rtlsdrHandler (frequency,
	                             rd  -> ppmCorrection,
	                             rd  -> theGain,
	                             rd  -> autogain);
#elif
	   fprintf (stderr, "No device configured, fatal\n");
	   throw (33);
#endif
	} catch (int e) {
	   fprintf (stderr, "failing to get a device, fatal\n");
	   throw (e);
	}
	return NULL;
}

static
sdp_session_t *register_service (void) {
//uint32_t service_uuid_int [] = {0, 0, 0, 0xABCD};
uint8_t service_uuid_int [] =
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xab, 0xcd };

uint8_t rfcomm_channel		= 3;
const char *service_name	= "Jan's dab handler";
const char *svc_dsc		= "dab handler";
const char *service_prov	= "Lazy Chair computing";

uuid_t root_uuid, l2cap_uuid, rfcomm_uuid, svc_uuid, svc_class_uuid;
sdp_list_t *l2cap_list		= 0, 
	   *rfcomm_list		= 0,
	   *root_list		= 0,
	   *proto_list		= 0, 
	   *access_proto_list	= 0,
	   *svc_class_list	= 0,
	   *profile_list	= 0;
sdp_data_t *channel		= 0;
sdp_profile_desc_t profile;
sdp_record_t record		= { 0 };
sdp_session_t	*session	= 0;

//	set the general service ID
	sdp_uuid128_create (&svc_uuid, &service_uuid_int);
	sdp_set_service_id (&record, svc_uuid);

char str [256] = "";
	sdp_uuid2strn (&svc_uuid, str, 256);
	fprintf (stderr, "registering UUID %s\n", str);
//	set the service class
	sdp_uuid16_create (&svc_class_uuid, SERIAL_PORT_SVCLASS_ID);
	svc_class_list	= sdp_list_append (0, &svc_class_uuid);
	sdp_set_service_classes (&record, svc_class_list);
//
//	set the Bluetooth profile information
	sdp_uuid16_create (&profile. uuid, SERIAL_PORT_PROFILE_ID);
	profile. version	= 0x100;
	profile_list		= sdp_list_append (0, &profile);
	sdp_set_profile_descs (&record, profile_list);

//	make the service record publicly browsable
	sdp_uuid16_create (&root_uuid, PUBLIC_BROWSE_GROUP);
	root_list		= sdp_list_append (0, &root_uuid);
	sdp_set_browse_groups (&record, root_list);

//	set l2cap information
	sdp_uuid16_create (&l2cap_uuid, L2CAP_UUID);
	l2cap_list		= sdp_list_append (0, &l2cap_uuid);
	proto_list		= sdp_list_append (0, l2cap_list);
 
//	set rfcomm information
	sdp_uuid16_create (&rfcomm_uuid, RFCOMM_UUID);
	channel			= sdp_data_alloc (SDP_UINT8, &rfcomm_channel);
	rfcomm_list		= sdp_list_append (0, &rfcomm_uuid);
	sdp_list_append (rfcomm_list, channel);
	sdp_list_append (proto_list, rfcomm_list);

//	attach protocol information to service record
	access_proto_list	= sdp_list_append (0, proto_list);
	sdp_set_access_protos (&record, access_proto_list);
//
//
	sdp_set_info_attr (&record, service_name, service_prov, svc_dsc);

//	connect to the local SDP server, register the service record, and 
//	disconnect
//	session = sdp_connect (BDADDR_ANY, BDADDR_LOCAL, SDP_RETRY_IF_BUSY );
//	C++ compiler does not like BDADDR_ANY and BDADDR_LOCAL
	bdaddr_t tmp_1	= (bdaddr_t) {{0, 0, 0, 0, 0, 0}};
	bdaddr_t tmp_2	= (bdaddr_t) {{0, 0, 0, 0xff, 0xff, 0xff}};
	session = sdp_connect (&tmp_1, &tmp_2, SDP_RETRY_IF_BUSY );
	sdp_record_register (session, &record, 0);

//	cleanup
	sdp_data_free (channel);
	sdp_list_free (l2cap_list, 0);
	sdp_list_free (rfcomm_list, 0);
	sdp_list_free (root_list, 0);
	sdp_list_free (access_proto_list, 0);
	sdp_list_free (svc_class_list, 0);
	sdp_list_free (profile_list, 0);

	return session;
}
