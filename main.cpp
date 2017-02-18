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
 *	Main program
 */
#include	<unistd.h>
#include	<signal.h>
#include	<getopt.h>
#include	<atomic>
#include	"dab-constants.h"
#include	"audiosink.h"
#include	"virtual-input.h"
#ifdef	HAVE_DABSTICK
#include	"dabstick.h"
#endif
#ifdef	HAVE_SDRPLAY
#include	"sdrplay.h"
#endif
#ifdef	HAVE_AIRSPY
#include	"airspy-handler.h"
#endif
#include	"radio.h"

Radio	*myRadio	= NULL;
std::atomic<bool> run (false);


static void sighandler (int signum) {
        fprintf (stderr, "Signal caught, terminating!\n");
	run. store (false);
}

virtualInput     *setDevice		(std::string s);
void		setModeParameters	(uint8_t Mode);
DabParams       dabModeParameters;

int	main (int argc, char **argv) {
// Default values
uint8_t		dabMode		= 1;	// illegal value
std::string	dabDevice	= "";
std::string	dabBand		= "BAND III";
std::string	dabChannel	= "11C";
std::string	programName	= "Classic FM";
int		dabGain		= 50;	// scale = 0 .. 100
std::string	soundChannel	= "default";
int16_t		latency		= 4;
int	opt;
struct sigaction sigact;

virtualInput	*inputDevice;
audioSink	*soundOut;
//
	while ((opt = getopt (argc, argv, "i:D:M:B:C:P:G:A:L:")) != -1) {
	   switch (opt) {

	      case 'D':
	         dabDevice	= optarg;
	         break;

	      case 'M':
	         dabMode	= atoi (optarg);
	         if (!(dabMode == 1) || (dabMode == 2) || (dabMode == 4))
	            dabMode = 1; 
	         break;

	      case 'B':
	         dabBand 	= optarg;
	         break;

	      case 'C':
	         dabChannel	= std::string (optarg);
	         break;

	      case 'P':
	         programName	= optarg;
	         break;

	      case 'G':
	         dabGain	= atoi (optarg);
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
	bool	success;
	bool	err;

//	The real radio is

//	the name of the device is passed on from the main program
//	it better be available
	inputDevice	= setDevice (dabDevice);
	if (inputDevice	== NULL) {
	   fprintf (stderr, "NO VALID DEVICE, GIVING UP\n");
	   exit (2);
	}
//
	soundOut	= new audioSink	(latency, soundChannel, &err);
	if (err) {
	   fprintf (stderr, "no valid sound channel, fatal\n");
	   exit (3);
	}

	setModeParameters (dabMode);
	myRadio = new Radio (inputDevice,
	                     soundOut,
	                     &dabModeParameters,
	                     dabBand,
	                     dabChannel,
	                     programName,
	                     dabGain,
	                     &success);
	if (!success)
	   exit (2);
	fflush (stdout);
	fflush (stderr);
//	delete myRadio;
	exit (1);
}


virtualInput	*setDevice (std::string s) {
bool	success;
virtualInput	*inputDevice;

#ifdef HAVE_AIRSPY
	if (s == "airspy") {
	   inputDevice	= new airspyHandler (&success, 80, Mhz (220));
	   fprintf (stderr, "devic selected\n");
	   if (!success) {
	      delete inputDevice;
	      return NULL;
	   }
	   else 
	      return inputDevice;
	}
	else
#endif
#ifdef	HAVE_SDRPLAY
	if (s == "sdrplay") {
	   inputDevice	= new sdrplay (&success, 60, Mhz (220));
	   if (!success) {
	      delete inputDevice;
	      return NULL;
	   }
	   else 
	      return inputDevice;
	}
	else
#endif
#ifdef	HAVE_DABSTICK
	if (s == "dabstick") {
	   inputDevice	= new dabStick (&success, 75, KHz (220000));
	   if (!success) {
	      delete inputDevice;
	      return NULL;
	   }
	   else
	      return inputDevice;
	}
	else
#endif
    {	// s == "no device"
//	and as default option, we have a "no device"
	   inputDevice	= new virtualInput ();
	}
	return NULL;
}

///	the values for the different Modes:
void	setModeParameters (uint8_t Mode) {

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

