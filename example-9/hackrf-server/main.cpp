#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming
 *
 *    This file is part of the hackrf transmitter
 *
 *    hackrf transmitter is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    hackrf transmitter is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with hackrf transmitter; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<stdint.h>
#include	"iq-input.h"
#include	"hackrf-server.h"

//	to do:
//	adding handling parameters, i.e. port, url rate
//

void	printHelp	(void) {
	fprintf (stderr, "hackrf iq transmitter\n"
"\toptions are:\n"
"\tf number\t: use number as initial frequency setting (in KHz)\n"
"\tp number\t: use number as port number for the input\n"
"\tr number\t: use number as input sample rate\n"
"\tu number\t: upswing factor to get a reasonable transmissionrate\n"
"\th\t:print this message\n");
}

std::atomic<bool>	running;

#define	MHz(x)	(x * 1000000)

int	main (int argc, char **argv) {
hackrf_server	*myTransmitter;
iq_Input	*myInputHandler;
int	opt;
int	port		= 8765;
int	vfoFrequency	= MHz (110);
int	rate		= 192000;
int	gain		= 35;
int	upFactor	= 1;
int	transmissionRate;
RingBuffer<uint8_t> *dataBuffer	= new RingBuffer<uint8_t> (8 * 32768);

        while ((opt = getopt (argc, argv, "f:p:r:g:u:h:")) != -1) {
           switch (opt) {

              case 'f':
	         vfoFrequency	= atoi (optarg);
	         break;

	      case 'p':
                 port		= atoi (optarg);
                 break;

	      case 'r':
	         rate		= atoi (optarg);
	         break;

	      case 'g':
	         gain		= atoi (optarg);
	         break;

	      case 'u':
	         upFactor	= atoi (optarg);
	         break;

	      case 'h':
              default:
	         printHelp ();
	         exit (1);
           }
        }

	if (upFactor * rate < 4 * 96000)
	   transmissionRate	= 4 * 96000;
	else
	   transmissionRate	= upFactor * rate;
	try {
	   myInputHandler	= new iq_Input (port, dataBuffer);
	   myTransmitter	= new hackrf_server (vfoFrequency,
	                                      rate,
	                                      gain,
	                                      transmissionRate,
	                                      dataBuffer);
	} catch (int e) {
	   fprintf (stderr, "setting up transmitter failed, fatal\n");
	   exit (1);
	}

	myTransmitter	-> start ();
	myInputHandler	-> start ();
	running. store (true);
	while (running. load ())
	   usleep (100000);
/*
 *	done:
 */
	myTransmitter	-> stop ();
	myInputHandler	-> stop ();
	fflush (stdout);
        fflush (stderr);
}

