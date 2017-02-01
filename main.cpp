#
/*
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
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
 *	Main program
 */
#include	<unistd.h>
#include	"dab-constants.h"
#include	"radio.h"


int	main (int argc, char **argv) {
Radio	*myRadio;
// Default values
uint8_t		dabMode		= 1;	// illegal value
std::string	dabDevice	= "";
std::string	dabBand		= "BAND III";
std::string	dabChannel	= "11C";
std::string	programName	= "Classic FM";
int		dabGain		= 50;	// scale = 0 .. 100
std::string	soundChannel	= "default";
int	opt;
//
	while ((opt = getopt (argc, argv, "i:D:M:B:C:P:G:A:")) != -1) {
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
	      default:
	         break;
	   }
	}
//
//	The real radio is
	myRadio = new Radio (dabDevice,
	                     dabMode,
	                     dabBand,
	                     dabChannel,
	                     programName,
	                     dabGain,
	                     soundChannel);

	fflush (stdout);
	fflush (stderr);
	delete myRadio;
	exit (1);
}

