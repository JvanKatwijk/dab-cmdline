#
/*
 *    Copyright (C) 2015, 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the  SDR-J (JSDR).
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are acknowledged.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include	"dab-constants.h"
#include	"radio.h"
#include	"audiosink.h"
#ifdef	HAVE_DABSTICK
#include	"dabstick.h"
#endif
#ifdef	HAVE_SDRPLAY
#include	"sdrplay.h"
#endif
#ifdef	HAVE_AIRSPY
#include	"airspy-handler.h"
#endif

extern	bool	running;
/**
  *	The "radio" class is reminiscent of the time that
  *	it "was" the GUI class.
  */
	Radio::Radio (std::string	device,
	              uint8_t		dabMode,
	              std::string	dabBand,
	              std::string	channel,
	              std::string	programName,
	              int		gain,
	              std::string	soundChannel,
	              int16_t		latency
	              bool		*OK) {
int16_t	latency;
bool	err;

	*OK		= false;
	running		= false;
//	Before printing anything, we set
	setlocale (LC_ALL, "");
	fprintf (stderr, "looking for channel %s\n", channel. c_str ());
//	
//	the name of the device is passed on from the main program
//	it better be available
	if (!setDevice (device)) {
	   fprintf (stderr, "NO VALID DEVICE, GIVING UP\n");
	   return;
	}
//
	inputDevice	-> setGain (gain);
	
/**	threshold is used in the phaseReference class 
  *	as threshold for checking the validity of the correlation result
  */
	threshold		= 3;
//
//	latency is used to allow different settings for different
//	situations wrt the output buffering
	this	-> latency	= latency;
//
	soundOut		= new audioSink	(latency, soundChannel, &err);
	if (err) {
	   fprintf (stderr, "no valid sound channel, fatal\n");
	   return;
	}

	this	-> dabBand		= dabBand == "BAND III" ?
	                                         BAND_III : L_BAND;
	
	setModeParameters (dabMode);
	this	-> requestedChannel	= channel;
	this	-> requestedProgram	= programName;
/**
  *	The actual work is done elsewhere: in ofdmProcessor
  *	and ofdmDecoder for the ofdm related part, ficHandler
  *	for the FIC's and mscHandler for the MSC.
  */
	my_ficHandler	= new ficHandler (&theEnsemble);
	my_mscHandler	= new mscHandler (this,
	                                  &dabModeParameters,
                                          soundOut);
/**
  *	The default for the ofdmProcessor depends on
  *	the input device, note that in this setup the
  *	device is selected on start up and cannot be changed.
  */
	my_ofdmProcessor = new ofdmProcessor   (inputDevice,
	                                        &dabModeParameters,
	                                        my_mscHandler,
	                                        my_ficHandler,
	                                        threshold,
	                                        3);
	if (!setChannel (channel)) {
	   fprintf (stderr, "selecting channel %s failed, fatal\n",
	                          channel. c_str ());
	   return;
	}

	int r = inputDevice	-> restartReader ();
	if (!r) {
	   fprintf (stderr, "Opening  input stream failed, fatal\n");
	   return;
	}

	inputDevice	-> setGain (gain);
	sleep (15);

	if (!theEnsemble. ensembleExists ()) {
	   fprintf (stderr, "could not find useful data in channel %s",
	                               requestedChannel. c_str ());
	   return;
	}
//
//	Eigenlijk zou het bovenstaande in de init van main moeten
//	staan, en een soort "gui handler" het restje moeten doen
//
void	RadioInterface::
	fprintf (stderr, "trying to open program %s\n",
	                               requestedProgram. c_str ()); 
	if (!setService (theEnsemble. findService (requestedProgram))) 
	   return;
//
//	When here, we actally do not have to do anything but wait
	running	= true;
	*OK		= true;
//
//	OK, here we start a listener that tells us when to change
//	one of the settings
//	a. gain
//	b. next/previous program
	while (running) {
	   std::unique_lock<std::mutex> locker (g_lockqueue);
	   auto now = std::chrono::system_clock::now ();
           g_queuecheck. wait_until (locker, now + 100ms);
	  
	   while (!labelQueue. empty ()) {
	      labelMutex. lock ();
	      fprintf (stderr, "%s\r", labelQueue. front (). c_str ());
	      labelQueue. pop_front ();
	      labelMutex. unlock ();
	   }
	}
}

	Radio::~Radio (void) {
	inputDevice		-> stopReader ();	// might be concurrent
	my_mscHandler		-> stopHandler ();	// might be concurrent
	my_ofdmProcessor	-> stop ();	// definitely concurrent
	soundOut		-> stop ();
//	
//	everything should be halted by now
	delete		my_ficHandler;
	delete		my_mscHandler;
	delete		my_ofdmProcessor;
	delete		soundOut;
}

void	Radio::stop	(void) {
	running		= false;
}

void	Radio::showLabel (const std::string s) {
	std::unique_lock<std::mutex> locker (g_lockqueue);
	labelMutex. lock ();
	labelQueue. insert (labelQueue. cend (), s);
	labelMutex. unlock ();
	g_queuecheck. notify_one ();
}

//
///	the values for the different Modes:
void	Radio::setModeParameters (uint8_t Mode) {

	if (Mode == 2) {
	   dabModeParameters. dabMode	= 2;
	   dabModeParameters. L		= 76;		// blocks per frame
	   dabModeParameters. K		= 384;		// carriers
	   dabModeParameters. T_null	= 664;		// null length
	   dabModeParameters. T_F	= 49152;	// samples per frame
	   dabModeParameters. T_s	= 638;		// block length
	   dabModeParameters. T_u	= 512;		// useful part
	   dabModeParameters. guardLength	= 126;
	   dabModeParameters. carrierDiff	= 4000;
	} else
	if (Mode == 4) {
	   dabModeParameters. dabMode		= 4;
	   dabModeParameters. L			= 76;
	   dabModeParameters. K			= 768;
	   dabModeParameters. T_F		= 98304;
	   dabModeParameters. T_null		= 1328;
	   dabModeParameters. T_s		= 1276;
	   dabModeParameters. T_u		= 1024;
	   dabModeParameters. guardLength	= 252;
	   dabModeParameters. carrierDiff	= 2000;
	} else 
	if (Mode == 3) {
	   dabModeParameters. dabMode		= 3;
	   dabModeParameters. L			= 153;
	   dabModeParameters. K			= 192;
	   dabModeParameters. T_F		= 49152;
	   dabModeParameters. T_null		= 345;
	   dabModeParameters. T_s		= 319;
	   dabModeParameters. T_u		= 256;
	   dabModeParameters. guardLength	= 63;
	   dabModeParameters. carrierDiff	= 2000;
	} else {	// default = Mode I
	   dabModeParameters. dabMode		= 1;
	   dabModeParameters. L			= 76;
	   dabModeParameters. K			= 1536;
	   dabModeParameters. T_F		= 196608;
	   dabModeParameters. T_null		= 2656;
	   dabModeParameters. T_s		= 2552;
	   dabModeParameters. T_u		= 2048;
	   dabModeParameters. guardLength	= 504;
	   dabModeParameters. carrierDiff	= 1000;
	}
}

struct dabFrequencies {
	const char	*key;
	int	fKHz;
};

struct dabFrequencies bandIII_frequencies [] = {
{"5A",	174928},
{"5B",	176640},
{"5C",	178352},
{"5D",	180064},
{"6A",	181936},
{"6B",	183648},
{"6C",	185360},
{"6D",	187072},
{"7A",	188928},
{"7B",	190640},
{"7C",	192352},
{"7D",	194064},
{"8A",	195936},
{"8B",	197648},
{"8C",	199360},
{"8D",	201072},
{"9A",	202928},
{"9B",	204640},
{"9C",	206352},
{"9D",	208064},
{"10A",	209936},
{"10B", 211648},
{"10C", 213360},
{"10D", 215072},
{"11A", 216928},
{"11B",	218640},
{"11C",	220352},
{"11D",	222064},
{"12A",	223936},
{"12B",	225648},
{"12C",	227360},
{"12D",	229072},
{"13A",	230748},
{"13B",	232496},
{"13C",	234208},
{"13D",	235776},
{"13E",	237488},
{"13F",	239200},
{NULL, 0}
};

struct dabFrequencies Lband_frequencies [] = {
{"LA", 1452960},
{"LB", 1454672},
{"LC", 1456384},
{"LD", 1458096},
{"LE", 1459808},
{"LF", 1461520},
{"LG", 1463232},
{"LH", 1464944},
{"LI", 1466656},
{"LJ", 1468368},
{"LK", 1470080},
{"LL", 1471792},
{"LM", 1473504},
{"LN", 1475216},
{"LO", 1476928},
{"LP", 1478640},
{NULL, 0}
};

//	These tables are not used in the command line version
static 
const char *table12 [] = {
"none",
"news",
"current affairs",
"information",
"sport",
"education",
"drama",
"arts",
"science",
"talk",
"pop music",
"rock music",
"easy listening",
"light classical",
"classical music",
"other music",
"wheather",
"finance",
"children\'s",
"factual",
"religion",
"phone in",
"travel",
"leisure",
"jazz and blues",
"country music",
"national music",
"oldies music",
"folk music",
"entry 29 not used",
"entry 30 not used",
"entry 31 not used"
};

const char *Radio::get_programm_type_string (uint8_t type) {
	if (type > 0x40) {
	   fprintf (stderr, "GUI: programmtype wrong (%d)\n", type);
	   return (table12 [0]);
	}

	return table12 [type];
}

static
const char *table9 [] = {
"unknown",
"Albanian",
"Breton",
"Catalan",
"Croatian",
"Welsh",
"Czech",
"Danish",
"German",
"English",
"Spanish",
"Esperanto",
"Estonian",
"Basque",
"Faroese",
"French",
"Frisian",
"Irish",
"Gaelic",
"Galician",
"Icelandic",
"Italian",
"Lappish",
"Latin",
"Latvian",
"Luxembourgian",
"Lithuanian",
"Hungarian",
"Maltese",
"Dutch",
"Norwegian",
"Occitan",
"Polish",
"Postuguese",
"Romanian",
"Romansh",
"Serbian",
"Slovak",
"Slovene",
"Finnish",
"Swedish",
"Tuskish",
"Flemish",
"Walloon"
};

const char *Radio::get_programm_language_string (uint8_t language) {
	if (language > 43) {
	   fprintf (stderr, "GUI: wrong language (%d)\n", language);
	   return table9 [0];
	}
	return table9 [language];
}

//
//	Not used by this version
void	Radio::set_fineCorrectorDisplay (int v) {
	fineCorrector = v;
}

//	Not used by this version
void	Radio::set_coarseCorrectorDisplay (int v) {
	coarseCorrector = v * kHz (1);
}

/**
  */
bool	Radio::setChannel (std::string s) {
int16_t	i;
struct dabFrequencies *finger;
int32_t	tunedFrequency;

	tunedFrequency		= 0;
	if (dabBand == BAND_III)
	   finger = bandIII_frequencies;
	else
	   finger = Lband_frequencies;

	for (i = 0; finger [i]. key != NULL; i ++) {
	   if (std::string (finger [i]. key) == s) {
	      tunedFrequency	= KHz (finger [i]. fKHz);
	      break;
	   }
	}

	if (tunedFrequency == 0) {
	   fprintf (stderr, "could not find a legal frequency for channel %s\n",
	                                     s. c_str ());
	   return false;
	}

	fprintf (stderr, "frequency = %d\n", tunedFrequency);
	inputDevice		-> setVFOFrequency (tunedFrequency);
	return true;
}

//	Note that the audiodata or the packetdata contains quite some
//	info on the service (i.e. rate, address, etc)
//	Here we only support audio services.
bool	Radio::setService (std::string name) {
audiodata d;

	switch (my_ficHandler -> kindofService (name)) {
	   case AUDIO_SERVICE:
	     my_ficHandler	-> dataforAudioService (name, &d);
	     if (d. defined) {
	        my_mscHandler	-> set_audioChannel (&d);
	        fprintf (stderr, "valid audio service %s\n", name. c_str ());
	        return true;
	     }
	     else {
	        fprintf (stderr, "Insufficient data to execute service %s",
	                                           name. c_str ());
	        return false;
	     }
	     break;
//
//	For the command line version, we do not have a data service
	   case PACKET_SERVICE:
	      fprintf (stderr,
	               "Data services not supported in this version\n");
	      return false;

	   default:
	      fprintf (stderr,
	               "kind of service %s unclear, fatal\n", name. c_str ());
	      return false;
	}
	return false;
}

//
bool	Radio::setDevice (std::string s) {
bool	success;
#ifdef HAVE_AIRSPY
	if (s == "airspy") {
	   inputDevice	= new airspyHandler (&success, 80, Mhz (220));
	   if (!success) {
	      delete inputDevice;
	      inputDevice = new virtualInput ();
	      return false;
	   }
	   else 
	      return true;
	}
	else
#endif
#ifdef	HAVE_SDRPLAY
	if (s == "sdrplay") {
	   inputDevice	= new sdrplay (&success, 60, Mhz (220));
	   if (!success) {
	      delete inputDevice;
	      inputDevice = new virtualInput ();
	      return false;
	   }
	   else 
	      return true;
	}
	else
#endif
#ifdef	HAVE_DABSTICK
	if (s == "dabstick") {
	   inputDevice	= new dabStick (&success, 75, KHz (220000));
	   if (!success) {
	      delete inputDevice;
	      inputDevice = new virtualInput ();
	      return false;
	   }
	   else
	      return true;
	}
	else
#endif
    {	// s == "no device"
//	and as default option, we have a "no device"
	   inputDevice	= new virtualInput ();
	}
	return false;
}

