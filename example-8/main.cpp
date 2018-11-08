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
#include	<map>
#include        <pthread.h>
#include	"dab-api.h"
#include	"audiosink.h"
#include	"config.h"
#include	"band-handler.h"
#include	"protocol.h"
#include	"sdrplay-handler.h"
#include	"radiodata.h"

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
typedef std::pair <std::string, std::string> mapElement;
static
std::map<std::string, std::string> serviceMap;

static
void	buildServiceList (bool fresh);

static
int    handleRequest (char *buf, int bufLen);

static
sdp_session_t *register_service (void);

static
void	set_audioLevel	(int);
//
//      This function is called from - most likely -
//      different threads from within the library
static
void    string_Writer (uint8_t code, std::string theText) {
int     len     = theText. length ();
int     i;
char	message [len + 3 + 1];
        message [0]     = (char)code;
        message [1]     = (len >> 8) & 0xFF;
        message [2]     = (len + 1) & 0xFF;
        for (i = 0; i < len; i ++)
           message [3 + i] = theText [i];
        message [len + 3] = (uint8_t)0;
//	locker. lock ();
        write (client, message, len + 3 + 1);
//	locker. unlock ();
}
//
//	for some keys we only send a small integer value
static
void	int_Writer (uint8_t code, int8_t v) {
char	message [5];
	message [0]	= (char)code;
	message [1]	= 0;
	message [2]	= 2;
	message [3]	= v;
//	locker. lock ();
	write (client, message, 5);
//	locker. unlock ();
}

static
std::atomic<bool>timeSynced;

static
std::atomic<bool>timesyncSet;

static
std::atomic<bool>ensembleRecognized;

static
std::atomic<bool> newEnsemble;
static
audioBase	*soundOut	= NULL;

void		handleSettings (radioData *rd);

static void sighandler (int signum) {
        fprintf (stderr, "Signal caught, terminating!\n");
	exit (21);
}

static
void	syncsignalHandler (bool b, void *userData) {
	timeSynced. store (b);
	timesyncSet. store (true);
	if (!b)
	   string_Writer (Q_TEXT_MESSAGE,
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
}

static
bool	scanning	= false;
static
void	programnameHandler (std::string s, int SId, void *userdata) {
audiodata ad;

	if (!scanning)
	   return;
	for (std::map<std::string, std::string>
	                              ::iterator it = serviceMap. begin ();
	             it != serviceMap. end(); ++it)
	   if (it -> first == s)
	      return;

	dataforAudioService (theRadio, s. c_str (), &ad, 0);
	if (ad. defined) {
	   serviceMap. insert (mapElement (s,
	                        theBandHandler -> currentChannel ()));
	}
}

static
void	programdataHandler (audiodata *d, void *ctx) {
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
	string_Writer (Q_TEXT_MESSAGE, localString);
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
	int_Writer (Q_SIGNAL_QUALITY, (char)q);
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
	theBandHandler	= new bandHandler ();
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

	
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;

	int32_t frequency	= 220000000;	// default
	try {
	   theDevice	=  new sdrplayHandler (frequency,
	                                       my_radioData. ppmCorrection,
	                                       my_radioData. GRdB,
	                                       my_radioData. lnaState,
	                                       my_radioData. autoGain);
	}
	catch (int e) {
	   fprintf (stderr, "No device\n");
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

	scanning	= true;
	buildServiceList (true);
	scanning	= false;
	while (true) {
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
//
//	send out the collected service names
	
	   for (std::map<std::string, std::string>
	                              ::iterator it = serviceMap. begin ();
	             it != serviceMap. end(); ++it) {
	      string_Writer (Q_SERVICE_NAME, it -> first);
	      usleep (1);
	   }
	   
           while (true) {
              int bytes_read;
              char buf [1024];
              bytes_read = read (client, buf, sizeof (buf));
              if (bytes_read > 0) {
                 if (handleRequest (buf, bytes_read) == 0)
	            break;
	      } else {
                 fprintf (stderr, "read gives back %d\n", bytes_read);
                 break;
	      }
	   }
	   theDevice	-> stopReader ();
	   dabStop (theRadio);
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
	rd	-> GRdB			= 35;	
	rd	-> lnaState		= 3;
	rd	-> autoGain		= false;
	rd	-> soundChannel		= "default";
	rd	-> latency		= 10;
	rd	-> waitingTime		= 10;

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

int	handleRequest (char *lbuf, int bufLen) {
int	starter	= 0;
	while (starter < bufLen) {
	   switch (lbuf [starter + 0]) {
	      case Q_QUIT:
	         fprintf (stderr, "quit request\n");
	         return 0;

	      case Q_IF_GAIN_REDUCTION:
	         my_radioData. GRdB = lbuf [starter + 3];
	         theDevice      -> set_ifgainReduction (my_radioData. GRdB);
	         break;

	      case Q_LNA_STATE:
	         my_radioData. lnaState = lbuf [starter + 3];
	         theDevice      -> set_lnaState (my_radioData. lnaState);
	         break;

	      case Q_AUTOGAIN:
	         my_radioData. autoGain = lbuf [starter + 3] != 0;
	         theDevice      -> set_autogain (my_radioData. autoGain);
	         break;

	      case Q_SOUND_LEVEL:
	         set_audioLevel (lbuf [starter + 3]);
	         break;

	      case Q_RESET:
	         return 1;

	      case Q_SERVICE:
	         {  std::string channel	= "";
	            my_radioData. serviceName =
	                          std::string ((char *)(&(lbuf [starter + 3])));
	            fprintf (stderr, "service request for %s\n",
	                              my_radioData. serviceName. c_str ());

	            for (std::map<std::string, std::string>
	                              ::iterator it = serviceMap. begin ();
	                 it != serviceMap. end(); ++it) {
	               if (it -> first == my_radioData. serviceName) {
	                  channel = it -> second;
	                  break;
	               }
	            }
	            if (channel == "") {
	               string_Writer (Q_NO_SERVICE, "service not found");
	               break;
	            }

	            if (channel !=  theBandHandler -> currentChannel ()) {
	               theDevice -> stopReader ();
	               dabStop (theRadio);
	               theBandHandler -> setChannel (channel);
                       int32_t frequency =
                                theBandHandler -> Frequency ();
                       theDevice    -> setVFOFrequency (frequency);
                       theDevice    -> restartReader ();
                       timesyncSet.         store (false);
                       timeSynced.          store (false);
                       int timeSyncTime         = 4;
	               ensembleRecognized.       store (false);
	               dabStartProcessing (theRadio);
	               while (!timesyncSet. load () && (--timeSyncTime >= 0))
	                  sleep (1);
	               if (!ensembleRecognized. load ()) {
	                  string_Writer (Q_NO_SERVICE, "service not found");
	                  break;
	               }
	            }

	            if (is_audioService (theRadio,
	                              my_radioData. serviceName. c_str ())) {
	               audiodata ad;
	               dataforAudioService (theRadio,
	                                 my_radioData. serviceName. c_str (),
	                                 &ad, 0);
	               if (!ad. defined) {
	                  std::cerr << "sorry  we cannot handle service " <<
	                                    my_radioData. serviceName << "\n";
	               }
	               else {
	                  dabReset_msc (theRadio);
	                  set_audioChannel (theRadio, &ad);
	               }
	            }
	         }
	         break;

	      default:
	         fprintf (stderr, "Message %o not understood\n",
	                                lbuf [starter + 0]);
	         break;
	   }
	   starter += 3 + lbuf [2];
	}
	return 1;
}

static
sdp_session_t *register_service (void) {
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
//	so we had to reinvent a number of wheels
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

static
void	set_audioLevel (int offset) {
char	command [255];
	if (offset == 0)
	   return;

	sprintf (command, "amixer set PCM -- %d%%", (offset + 10) * 5);
	system (command);
}

static
bool	searchable (std::string channel) {
	return true;
}

static
void	markChannels (bool flag, std::string Channel) {
}

static
void	buildServiceList (bool fresh) {
std::string	theChannel	= "5A";
std::string	startChannel	= "5A";
//
//	be sure that the serviceList is empty
        serviceMap. clear ();
//
//	here we really start
	while (true) {
	   if (fresh || searchable (theChannel)) {
	      theDevice -> stopReader ();
	      dabStop (theRadio);
	
	      theBandHandler -> setChannel (theChannel);
	      int32_t frequency =
                       theBandHandler -> Frequency ();
              theDevice    -> setVFOFrequency (frequency);
              theDevice    -> restartReader ();
	      timesyncSet.         store (false);
	      timeSynced.          store (false);
	      int timeSyncTime         = 4;
	      ensembleRecognized.	store (false);
	      dabStartProcessing (theRadio);
	      while (!timesyncSet. load () && (--timeSyncTime >= 0))
	         sleep (1);
	      dabStop (theRadio);
	      markChannels (ensembleRecognized. load (), theChannel);
	   }
           theChannel = theBandHandler -> nextChannel ();
           if (theChannel == startChannel)
              break;
	}
}



