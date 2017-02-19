#
/*
 *    Copyright (C) 2015, 2016
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the DAB-cmdline program.
 *    Many of the ideas as implemented in DAB-cmdline are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
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
 *	E X A M P L E  P R O G R A M
 *	for the DAB-library
 */
#include	<unistd.h>
#include	<signal.h>
#include	<getopt.h>
#include	"audiosink.h"
#include	"dab-api.h"
#include	<atomic>
//
//	we deal with some callbacks, so we some data that needs
//	to be accessed should be kept global
static
std::atomic<bool> run;

static
void		*theRadio	= NULL;

static
audioSink	*soundOut	= NULL;

std::string	programName	= "Classic FM";

static void sighandler (int signum) {
        fprintf (stderr, "Signal caught, terminating!\n");
	run. store (false);
}
//
//	This function is called whenever the dab engine has taken
//	some time to gather information from the FIC bloks
//	the Boolean b tells whether or not an ensemble has been
//	recognized, the names of the programs are in the 
//	ensemble
static
void	callblockHandler (std::list<std::string> ensemble, bool b) {
	if (!b) {
	   fprintf (stderr, "no ensemble, quitting");
	   run. store (false);
	   return;
	}
	dab_Service (theRadio, programName, NULL);
}

static
void	labelHandler (std::string dynamicLabel) {
	fprintf (stderr, "%s\n", dynamicLabel. c_str ());
}

static
void	pcmHandler (int16_t *buffer, int size, int rate) {
static bool isStarted	= false;

	if (!isStarted) {
	   soundOut	-> restart ();
	   isStarted	= true;
	}
	soundOut	-> audioOut (buffer, size, rate);
}

	
int	main (int argc, char **argv) {
// Default values
uint8_t		theMode		= 1;
std::string	theChannel	= "11C";
std::string	dabDevice	= "airspy";
dabBand		theBand		= BAND_III;
int		theGain		= 80;	// scale = 0 .. 100
std::string	soundChannel	= "default";
int16_t		latency		= 4;
int	opt;
struct sigaction sigact;

//
	while ((opt = getopt (argc, argv, "i:D:M:B:C:P:G:A:L:")) != -1) {
	   switch (opt) {
	      case 'D':
	         dabDevice	= optarg;
	         break;

	      case 'M':
	         theMode	= atoi (optarg);
	         if (!(theMode == 1) || (theMode == 2) || (theMode == 4))
	            theMode = 1; 
	         break;

	      case 'B':
	         theBand = std::string (optarg) == std::string ("L_BAND") ?
	                                     L_BAND : BAND_III;
	         break;

	      case 'C':
	         theChannel	= std::string (optarg);
	         break;

	      case 'P':
	         programName	= optarg;
	         break;

	      case 'G':
	         theGain	= atoi (optarg);
	         break;

	      case 'A':
	         soundChannel	= optarg;
	         break;

	      case 'L':
	         latency	= atoi (optarg);
	         break;

	      default:
	         break;
	   }
	}
//
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	bool	err;

	soundOut	= new audioSink	(latency, soundChannel, &err);
	if (err) {
	   fprintf (stderr, "no valid sound channel, fatal\n");
	   exit (3);
	}

	theRadio	= dab_initialize ("airspy", theMode, theBand,
		                                   pcmHandler, labelHandler);
	if (theRadio == NULL) {
	   fprintf (stderr, "sorry, no radio available, fatal\n");
	   exit (4);
	}

	dab_Gain	(theRadio, theGain);
	dab_Channel	(theRadio, theChannel);
	dab_run		(theRadio, callblockHandler);
	run. store (true);
	while (run. load ())
	   sleep (1);
	dab_stop (theRadio);
	dab_exit (&theRadio);
}

