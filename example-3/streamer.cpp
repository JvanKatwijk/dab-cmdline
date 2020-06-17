#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library
 *
 *    DAB library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"streamer.h"
#include        <sys/time.h>
#include        <time.h>
#include	<unistd.h>
#include	<stdlib.h>

static inline
int64_t         getMyTime       (void) {
struct timeval  tv;

        gettimeofday (&tv, NULL);
        return ((int64_t)tv. tv_sec * 1000000 + (int64_t)tv. tv_usec);
}

	streamer::streamer	(void) {
	theBuffer	= new RingBuffer <int16_t> (16 * 32768);
	running. store (false);
}

	streamer::~streamer	(void) {
	stop ();
	delete theBuffer;
}

void	streamer::stop		(void) {
	running. store (false);
	workerHandle. join ();
}

bool	streamer::isRunning	(void) {
	return running. load ();
}

bool    streamer::restart (void) {
        workerHandle = std::thread (&streamer::run, this);
        return true;
}
//
//
//	amount is the number of 16 bit value, i.e. 2 values per sample
void	streamer::addBuffer	(void *buffer, int amount, int elsize) {
	(void)elsize;
	theBuffer	-> putDataIntoBuffer (buffer, amount);
}

int16_t lbuf [2 * 4800];
void	streamer::run		(void) {
int	period		= 10000;	// msec
int64_t nextStop	= (int64_t)(getMyTime ());
	running. store (true);
	while (running. load ()) {
	   int a = theBuffer -> getDataFromBuffer (lbuf, 2 * 4800);
//	   fprintf (stderr, " a = %d\n", a);
	   if (a < 2 * 4800)
	      memset (&lbuf [a], 0, (4800 * 2 - a) * sizeof (int16_t));
	   nextStop	= nextStop + period;
	   fwrite (lbuf, 2 * 4800, 2, stdout);
	   if (nextStop - getMyTime () > 0)
	      usleep (nextStop - getMyTime ());
	}
}

