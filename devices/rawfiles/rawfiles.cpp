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
 * 	File reader:
 *	For the (former) files with 8 bit raw data from the
 *	dabsticks 
 */
#include        <stdio.h>
#include        <unistd.h>
#include        <stdlib.h>
#include        <fcntl.h>
#include        <sys/time.h>
#include        <time.h>
#include        <cstring>
#include        "rawfiles.h"

static inline
int64_t         getMyTime       (void) {
struct timeval  tv;

        gettimeofday (&tv, NULL);
        return ((int64_t)tv. tv_sec * 1000000 + (int64_t)tv. tv_usec);
}


#define	__BUFFERSIZE	16 * 32768
//
//
	rawFiles::rawFiles (std::string f, bool repeater) {
	fileName	= f;
	(void) repeater;
	_I_Buffer	= new RingBuffer<std::complex<float>>(__BUFFERSIZE);
	filePointer	= fopen (f. c_str (), "rb");
	if (filePointer == NULL) {
	   fprintf (stderr, "file %s cannot open\n", f. c_str ());
	   perror ("file ?");
	   delete _I_Buffer;
	   throw (31);
	}
	currPos		= 0;
	this	-> eofHandler	= nullptr;
	this	-> userData	= nullptr;
	running. store (false);
}

	rawFiles::rawFiles (std::string f,
	                    double fileOffsetInSeconds,
	                    device_eof_callback_t eofHandler,
	                    void * userData) {
	fileName	= f;
	_I_Buffer	= new RingBuffer<std::complex<float>>(__BUFFERSIZE);
	filePointer	= fopen (f. c_str (), "rb");
	if (filePointer == NULL) {
	   fprintf (stderr, "file %s cannot open\n", f. c_str ());
	   perror ("file ?");
	   delete _I_Buffer;
	   throw (31);
	}
	currPos = (int64_t)(fileOffsetInSeconds * 2048000.0 * 2.0 );
	fseek (filePointer, currPos, SEEK_SET);
	this	-> eofHandler	= eofHandler;
	this	-> userData	= userData;
	running. store (false);
}

	rawFiles::~rawFiles (void) {
	if (running. load ())
	   workerHandle. join ();
	running. store (false);
	fclose (filePointer);
	delete _I_Buffer;
}

bool	rawFiles::restartReader	(int32_t frequency) {
	(void)frequency;
	workerHandle = std::thread (&rawFiles::run, this);
	running. store (true);
	return true;
}

void	rawFiles::stopReader	(void) {
       if (running. load ())
           workerHandle. join ();
        running. store (false);
}

int32_t	rawFiles::getSamples	(std::complex<float> *V, int32_t size) {
int32_t	amount;

	if (filePointer == NULL)
	   return 0;

	while ((_I_Buffer -> GetRingBufferReadAvailable ()) < size)
	   usleep (100);

	amount	= _I_Buffer -> getDataFromBuffer (V, size);
	return amount;
}

int32_t	rawFiles::Samples (void) {
	return _I_Buffer -> GetRingBufferReadAvailable ();
}
//
//	The actual interface to the filereader is in a separate thread
//
void	rawFiles::run (void) {
int32_t	t, i;
std::complex<float>	*bi;
int32_t	bufferSize	= 32768;
int64_t	period;
int64_t	nextStop;
bool	eofReached	= false;

	running. store (true);
	period		= (32768 * 1000) / (2 * 2048);	// full IQś read
	fprintf (stderr, "Period = %ld\n", period);
	bi		= new std::complex<float> [bufferSize];
	nextStop	= getMyTime ();
	while (running. load ()) {
	   while (_I_Buffer -> WriteSpace () < bufferSize + 10) {
	      if (!running. load ())
	         break;
	      usleep (100);
	   }

	   nextStop += period;
	   t = readBuffer (bi, bufferSize);
	   if (t <= bufferSize) {
	      for (i = 0; i < bufferSize; i ++)
	          bi [i] = 0;
	      t = bufferSize;
	      eofReached	= true;
	   }

	   _I_Buffer -> putDataIntoBuffer (bi, t);
	   if (eofReached) {
	      eofReached = false;
	      if (eofHandler != nullptr) 
	         eofHandler (userData);
	   }
	   if (nextStop - getMyTime () > 0)
	      usleep (nextStop - getMyTime ());
	}
	fprintf (stderr, "taak voor replay eindigt hier\n");
}
/*
 *	length is number of uints that we read.
 */
int32_t	rawFiles::readBuffer (std::complex<float> *data, int32_t length) {
int32_t	n;
int	i;
uint8_t temp [2 * length];
	n = fread (temp,  sizeof (uint8_t), 2 * length, filePointer);
	for (i = 0; i < n / 2; i ++)
	   data [i] = std::complex <float> ((float)(temp [2 * i] - 128) / 128,
	                                    (float)(temp [2 * i + 1] - 128) / 128);
	currPos		+= n;
	if (n < length) {
	   fseek (filePointer, 0, SEEK_SET);
//	   fprintf (stderr, "End of file, restarting\n");
	}
	return	n / 2;
}

