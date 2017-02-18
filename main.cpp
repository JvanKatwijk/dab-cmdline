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
#include	"dab-constants.h"
#include	"radio.h"

Radio	*myRadio	= NULL;
bool	running		= false;

static void sighandler (int signum) {
        fprintf (stderr, "Signal caught, terminating!\n");
	running	= false;
}

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
//	The real radio is
	myRadio = new Radio (dabDevice,
	                     dabMode,
	                     dabBand,
	                     dabChannel,
	                     programName,
	                     dabGain,
	                     soundChannel,
	                     latency,
	                     &success);
	if (!success)
	   exit (2);
	fflush (stdout);
	fflush (stderr);
	delete myRadio;
	exit (1);
}


