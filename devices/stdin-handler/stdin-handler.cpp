#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library
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
 *
 * 	stdinHandler
 *	takes the input bytes from stdin
 */
#include        <stdio.h>
#include        <unistd.h>
#include        <stdlib.h>
#include        <fcntl.h>
#include        <sys/time.h>
#include        <time.h>
#include        <cstring>
#include        "stdin-handler.h"
#include	      "device-exceptions.h"

static inline
int64_t         getMyTime       (void) {
struct timeval  tv;

        gettimeofday (&tv, NULL);
        return ((int64_t)tv. tv_sec * 1000000 + (int64_t)tv. tv_usec);
}

#define	__BUFFERSIZE	16 * 32768
//
//
	stdinHandler::stdinHandler (void) {
	_I_Buffer	= new RingBuffer<std::complex<float>>(__BUFFERSIZE);

	filePointer	= stdin;
	if (filePointer == NULL) {
	   DEBUG_PRINT ("Cannot open stdin\n");
	   delete _I_Buffer;
	   throw OpeningFileFailed("stdin","idk shouldn't fail");
	}
	running. store (false);
}

	stdinHandler::~stdinHandler (void) {
	if (running. load ()) {
	   running. store (false);
	   workerHandle. join ();
	}
	fclose (filePointer);
	delete _I_Buffer;
}

bool	stdinHandler::restartReader	(int32_t frequency) {
	(void)frequency;
	workerHandle = std::thread (&stdinHandler::run, this);
	running. store (true);
	return true;
}


void	stdinHandler::stopReader	(void) {
	if (running. load ())
           workerHandle. join ();
        running. store (false);
}

int32_t	stdinHandler::getSamples	(std::complex<float> *V, int32_t size) {
int32_t	amount;

	if (filePointer == NULL)
	   return 0;

	while ((_I_Buffer -> GetRingBufferReadAvailable ()) < size)
	   usleep (100);

	amount	= _I_Buffer -> getDataFromBuffer (V, size);
	return amount;
}

int32_t	stdinHandler::Samples (void) {
	return _I_Buffer -> GetRingBufferReadAvailable ();
}
//
//	The actual interface to the filereader is in a separate thread
//	we read in fragments of 2 msec
void	stdinHandler::run (void) {
int32_t	t, i;
std::complex<float>	*bi;
uint8_t	*b2;
int32_t	bufferSize	= 2048 * 2;
int64_t	nextStop;

	running. store (true);
	bi		= new std::complex<float> [bufferSize];
	b2		= new uint8_t [bufferSize * 2];
	nextStop	= getMyTime ();
	while (running. load ()) {
	   while (_I_Buffer -> WriteSpace () < bufferSize + 10) {
	      if (!running. load ())
	         break;
	      usleep (100);
	   }

	   nextStop += period;
	   t = fread (b2, 1, 2 * bufferSize, filePointer);
	   for (i = 0; i < bufferSize; i ++) {
	      bi [i] = std::complex <float> (
	                            (b2 [2 * i    ] - 128) / 128.0,
	                            (b2 [2 * i + 1] - 128) / 128.0);
	   }

	   _I_Buffer -> putDataIntoBuffer (bi, t);
	   if (nextStop - getMyTime () > 0)
	      usleep (nextStop - getMyTime ());
	}

	delete [] b2;
	delete [] bi;
	DEBUG_PRINT ("taak voor replay eindigt hier\n");
}
