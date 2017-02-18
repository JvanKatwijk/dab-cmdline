#
/*
 *    Copyright (C) 2015, 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the  DAB-cmdline
 *    Many of the ideas as implemented in DAB-cmdline are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are acknowledged.
 *
 *    DAB-cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB-cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB-cmdline; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include	<unistd.h>
#include	<termios.h>
#include	<stdio.h>
#include	<sys/select.h>
#include	<atomic>
#include	"dab-constants.h"
#include	"radio.h"
#include	"audiosink.h"

extern	std::atomic<bool> run;


/**
  *	The "radio" class is reminiscent of the time that
  *	it "was" the GUI class.
  *	The first three parameters are in this setup immutable
  *	We are trying to setup something such that the others
  *	might be changed during operation
  */
	Radio::Radio (virtualInput	*inputDevice,
	              audioSink		*soundOut,
	              DabParams		*dabModeParameters,
	              std::string	dabBand,
	              std::string	channel,
	              std::string	programName,
	              int		gain,
	              bool		*OK) {

	this	-> inputDevice	= inputDevice;
	this	-> soundOut	= soundOut;
	this	-> dabModeParameters	= dabModeParameters;

	*OK		= false;
//	Before printing anything, we set
	setlocale (LC_ALL, "");
//	
	inputDevice	-> setGain (gain);
	
	threshold		= 3;
//
	this	-> dabBand		= dabBand == "BAND III" ?
	                                         BAND_III : L_BAND;
	
	this	-> requestedChannel	= channel;
	this	-> requestedProgram	= programName;
/**
  *	The actual work is done elsewhere: in ofdmProcessor
  *	and ofdmDecoder for the ofdm related part, ficHandler
  *	for the FIC's and mscHandler for the MSC.
  */
	my_ficHandler	= new ficHandler (&theEnsemble);
	my_mscHandler	= new mscHandler (this,
	                                  dabModeParameters,
                                          soundOut);

	my_ofdmProcessor = new ofdmProcessor   (inputDevice,
	                                        dabModeParameters,
	                                        my_mscHandler,
	                                        my_ficHandler,
	                                        threshold, 2);

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
//
//	we give the underlying system 10 seconds for finding an ensemble
	sleep (10);

	if (!theEnsemble. ensembleExists ()) {
	   fprintf (stderr, "could not find useful data in channel %s",
	                               requestedChannel. c_str ());
	   return;
	}
//
	if (!setService (theEnsemble. findService (requestedProgram))) 
	   return;
//
//	When here, we actally do not have to do anything but wait
	*OK		= true;
//
//	OK, here we start a listener that tells us when to change
//	one of the settings
//	a. gain
//	b. next/original program
	run. store (true);
	threadHandle    = std::thread (&Radio::listener, this);

	while (run. load ()) {
	   std::unique_lock<std::mutex> locker (g_lockqueue);
	   auto now = std::chrono::system_clock::now ();
           g_queuecheck. wait_until (locker, now + 100ms);
	  
	   while (!labelQueue. empty ()) {
	      labelMutex. lock ();
//	      fprintf (stderr, "%s\r", labelQueue. front (). c_str ());
	      labelQueue. pop_front ();
	      labelMutex. unlock ();
	   }

	   while (!commandQueue. empty ()) {
	      commandMutex. lock ();
	      switch (commandQueue. front ()) {
	         case 1:
	            this -> requestedProgram =
	                       theEnsemble. nextof (this -> requestedProgram);
	            if (!setService (this -> requestedProgram)) {
	               fprintf (stderr, "sorry, program not available\n");
	               setService (theEnsemble.
	                       findService (requestedProgram));
	            }
	            break;

	         case 2:
	            this -> requestedProgram = programName;
	            setService (theEnsemble.
	                       findService (requestedProgram));
	            break;

	         default:;
	      }
	      commandQueue. pop_front ();
	      commandMutex. unlock ();
	   }
	}
	threadHandle. join ();
	fprintf (stderr, "going to delete ofdmprocessor\n");
	delete	my_ofdmProcessor;
}

	Radio::~Radio (void) {
	inputDevice		-> stopReader ();	// might be concurrent
	my_mscHandler		-> stopHandler ();	// might be concurrent
//	
//	everything should be halted by now
	delete		my_ficHandler;
	delete		my_mscHandler;
	delete		my_ofdmProcessor;
}

char getch (void) {
char buf = 0;
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0)
                perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0)
                perror("tcsetattr ICANON");
        if (read (0, &buf, 1) < 0)
                perror ("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
	   perror ("tcsetattr ~ICANON");
        return (buf);
}

void	Radio::listener	(void) {
struct timeval tv;
fd_set	fds;
char	c;

	while (run.load ()) {
	   tv. tv_sec	= 0;
	   tv. tv_usec	= 0;
	   FD_ZERO (&fds);
	   FD_SET  (STDIN_FILENO, &fds);
	   select  (STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
	   if (FD_ISSET (0, &fds)) {
	      c = getch ();
	      if (c == '+')
	         setCommand (1);
	      else
	      if (c == '0')
	         setCommand (2);
	   }
	   usleep (20000);
	}
}

void	Radio::Stop	(void) {
	run. store (false);
}

void	Radio::showLabel (const std::string s) {
	std::unique_lock<std::mutex> locker (g_lockqueue);
	labelMutex. lock ();
	labelQueue. insert (labelQueue. cend (), s);
	labelMutex. unlock ();
	g_queuecheck. notify_one ();
}

void	Radio::setCommand (int c) {
std::unique_lock<std::mutex> locker (g_lockqueue);
        commandMutex. lock ();
        commandQueue. insert (commandQueue. cend (), c);
        commandMutex. unlock ();
        g_queuecheck. notify_one ();
}

//
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
	        fprintf (stderr, "selected %s\n", name. c_str ());
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

