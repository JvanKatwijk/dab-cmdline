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
#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<time.h>
#include	<cstring>
#include	"wavfiles.h"
#include	"device-exceptions.h"

static inline
int64_t		getMyTime	(void) {
struct timeval	tv;

	gettimeofday (&tv, NULL);
	return ((int64_t)tv. tv_sec * 1000000 + (int64_t)tv. tv_usec);
}

#define	__BUFFERSIZE	8 * 32768

	wavFiles::wavFiles (std::string f, bool repeater) {
SF_INFO *sf_info;

	fileName	= f;
	this	-> repeater	= repeater;
	this	-> eofHandler	= nullptr;
	this	-> userData	= nullptr;

	_I_Buffer	= new RingBuffer<std::complex<float>>(__BUFFERSIZE);

	sf_info		= (SF_INFO *)alloca (sizeof (SF_INFO));
	sf_info	-> format	= 0;
	filePointer	= sf_open (f. c_str (), SFM_READ, sf_info);
	if (filePointer == NULL) {
	   DEBUG_PRINT ("file %s no legitimate sound file\n",
	                                f. c_str ());
	   throw OpeningFileFailed(f.c_str(),"Not a legitimate sound file");
	}
	if ((sf_info -> samplerate != 2048000) ||
	    (sf_info -> channels != 2)) {
	   DEBUG_PRINT ("This is not a recorded DAB file, sorry\n");
	   sf_close (filePointer);
	   throw OpeningFileFailed(f.c_str(),"Not a DAB file");
	}
	currPos		= 0;
	running. store (false);
}

	wavFiles::wavFiles (std::string f,
	                    double fileOffsetInSeconds,
	                    device_eof_callback_t eofHandler,
	                    void * userData) {
SF_INFO *sf_info;

	fileName		= f;
	repeater		= false;
	this	-> eofHandler	= eofHandler;
	this	-> userData	= userData;
	_I_Buffer		= new RingBuffer<std::complex<float>>(__BUFFERSIZE);

	sf_info			= (SF_INFO *)alloca (sizeof (SF_INFO));
	sf_info	-> format	= 0;
	filePointer	= sf_open (f. c_str (), SFM_READ, sf_info);
	if (filePointer == NULL) {
	   DEBUG_PRINT ("file %s no legitimate sound file\n",
	                                f. c_str ());
	   throw OpeningFileFailed(f.c_str(),"Not a legitimate sound file");
	}

	if ((sf_info -> samplerate != 2048000) ||
	    (sf_info -> channels != 2)) {
	   DEBUG_PRINT ("This is not a recorded DAB file, sorry\n");
	   sf_close (filePointer);
	   throw OpeningFileFailed(f.c_str(),"Not a DAB file");
	}

	currPos = (int64_t)(fileOffsetInSeconds * 2048000.0 );
	sf_seek (filePointer, (sf_count_t)currPos, SEEK_SET);
	running. store (false);
}

	wavFiles::~wavFiles (void) {
	if (running. load ())
	   workerHandle. join ();
	running. store (false);
	sf_close (filePointer);
	delete _I_Buffer;
}

bool	wavFiles::restartReader	(int32_t frequency) {
	(void)frequency;
	workerHandle = std::thread (&wavFiles::run, this);
	running. store (true);
	return true;
}

void	wavFiles::stopReader	(void) {
	if (running. load ()) {
	   running. store (false);
           workerHandle. join ();
	}
	running. store (false);
}
//
//	size is in I/Q pairs
int32_t	wavFiles::getSamples	(std::complex<float> *V, int32_t size) {
int32_t	amount;
	if (filePointer == NULL)
	   return 0;
	if (!running. load ())
	   return 0;

	while (_I_Buffer -> GetRingBufferReadAvailable () < (uint32_t)size)
	   usleep (100);

	amount = _I_Buffer	-> getDataFromBuffer (V, size);
	return amount;
}

int32_t	wavFiles::Samples (void) {
	return _I_Buffer -> GetRingBufferReadAvailable ();
}
//
//	The actual interface to the filereader is in a separate thread

void	wavFiles::run (void) {
int32_t	t, i;
std::complex<float>	*bi;
int32_t	bufferSize	= 32768;
int64_t	period;
int64_t	nextStop;
bool	eofReached	= false;

	running. store (true);
	period		= (32768 * 1000) / 2048;	// full IQś read
	DEBUG_PRINT ("Period = %ld\n", period);
	bi		= new std::complex<float> [bufferSize];
	nextStop	= getMyTime ();
	while (running. load ()) {
	   while (_I_Buffer -> WriteSpace () < bufferSize) {
	      if (!running. load ())
	         break;
	      usleep (100);
	   }

	   nextStop += period;
	   t = readBuffer (bi, bufferSize);
	   if (t < bufferSize) {
	      eofReached	= true;
	      for (i = t; i < bufferSize; i ++)
	          bi [i] = 0;
	      t = bufferSize;
	   }
	   _I_Buffer -> putDataIntoBuffer (bi, bufferSize);
	   if (eofReached && this -> repeater) {
	      sf_seek (filePointer, (sf_count_t)0, SEEK_SET);
	      eofReached = false;
	   }
	   else
	   if (eofReached && (eofHandler != nullptr)) {
	      eofHandler (userData);
	      eofReached	= false;
	   }
	   else
	   if (eofReached)
	      break;
	   if (nextStop - getMyTime () > 0)
	      usleep (nextStop - getMyTime ());
	}
	DEBUG_PRINT ("taak voor replay eindigt hier\n");
	delete [] bi;
}
/*
 *	length is number of uints that we read.
 */
int32_t	wavFiles::readBuffer (std::complex<float> *data, int32_t length) {
int32_t	i, n;
float	temp [2 * length];

	n = sf_readf_float (filePointer, temp, length);
	if (n < length) {
	   sf_seek (filePointer, 0, SEEK_SET);
//	   DEBUG_PRINT ("End of file, restarting\n");
	}
	for (i = 0; i < n; i ++)
	   data [i] = std::complex<float> (temp [2 * i], temp [2 * i + 1]);
	return	n & ~01;
}
