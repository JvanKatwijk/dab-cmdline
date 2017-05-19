#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the DAB-library
 *    DAB-library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB-library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB-library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#
#include	"dab-constants.h"
#include	"dab-audio.h"
#include	"mp2processor.h"
#include	"mp4processor.h"
#include	"eep-protection.h"
#include	"uep-protection.h"
#include	<chrono>
//
//	As an experiment a version of the backend is created
//	that will be running in a separate thread. Might be
//	useful for multicore processors.
//
//	Interleaving is - for reasons of simplicity - done
//	inline rather than through a special class-object
//
//	fragmentsize == Length * CUSize
	dabAudio::dabAudio	(uint8_t	dabModus,
	                         int16_t	fragmentSize,
	                         int16_t	bitRate,
	                         bool		shortForm,
	                         int16_t 	protLevel,
	                         audioOut_t	soundOut,
	                         dataOut_t	dataOut,
	                         programQuality_t mscQuality,
	                         void		*ctx) {
int32_t i;

	this	-> dabModus		= dabModus;
	this	-> fragmentSize		= fragmentSize;
	this	-> bitRate		= bitRate;
	this	-> shortForm		= shortForm;
	this	-> protLevel		= protLevel;

	outV			= new uint8_t [bitRate * 24];
	interleaveData		= new int16_t *[16]; // max size
	for (i = 0; i < 16; i ++) {
	   interleaveData [i] = new int16_t [fragmentSize];
	   memset (interleaveData [i], 0, fragmentSize * sizeof (int16_t));
	}

	if (shortForm)
	   protectionHandler	= new uep_protection (bitRate,
	                                              protLevel);
	else
	   protectionHandler	= new eep_protection (bitRate,
	                                              protLevel);
//
	
	if (dabModus == DAB) 
	   our_dabProcessor = new mp2Processor (bitRate,
	                                        soundOut,
	                                        dataOut,
	                                        mscQuality, ctx);
	else
	if (dabModus == DAB_PLUS) 
	   our_dabProcessor = new mp4Processor (bitRate,
	                                        soundOut,
	                                        dataOut,
	                                        mscQuality, ctx);
	else		// cannot happen
	   our_dabProcessor = new dabProcessor ();

	fprintf (stderr, "we have now %s\n", dabModus == DAB_PLUS ? "DAB+" : "DAB");
	Buffer		= new RingBuffer<int16_t>(64 * 32768);
	start ();
}

	dabAudio::~dabAudio	(void) {
int16_t	i;
	if (running. load ()) {
	   running. store (false);
	   threadHandle. join ();
	}
	delete protectionHandler;
	delete our_dabProcessor;
	delete	Buffer;
	delete []	outV;
	for (i = 0; i < 16; i ++) 
	   delete[]  interleaveData [i];
	delete [] interleaveData;
}

void	dabAudio::start		(void) {
	running. store (true);
	threadHandle = std::thread (&dabAudio::run, this);
}

int32_t	dabAudio::process	(int16_t *v, int16_t cnt) {
int32_t	fr;
	   if (Buffer -> GetRingBufferWriteAvailable () < cnt)
	      fprintf (stderr, "dab-concurrent: buffer full\n");
	   while ((fr = Buffer -> GetRingBufferWriteAvailable ()) <= cnt) {
	      if (!running. load ())
	         return 0;
	      usleep (1);
	   }

	   Buffer	-> putDataIntoBuffer (v, cnt);
	   Locker. notify_all ();
	   return fr;
}

const	int16_t interleaveMap [] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
void	dabAudio::run	(void) {
int16_t	i, j;
int16_t	countforInterleaver	= 0;
int16_t	interleaverIndex	= 0;
uint8_t	shiftRegister [9];
int16_t	Data [fragmentSize];
int16_t	tempX [fragmentSize];

	while (running. load ()) {
	   while (Buffer -> GetRingBufferReadAvailable () <= fragmentSize) {
	      std::unique_lock <std::mutex> lck (ourMutex);
	      auto now = std::chrono::system_clock::now ();
	      Locker. wait_until (lck, now + std::chrono::milliseconds (1));	// 1 msec waiting time
	      if (!running. load ())
	         break;
	   }

	   if (!running. load ()) 
	      break;

	   Buffer	-> getDataFromBuffer (Data, fragmentSize);

	   for (i = 0; i < fragmentSize; i ++) {
	      tempX [i] = interleaveData [(interleaverIndex + 
	                                 interleaveMap [i & 017]) & 017][i];
	      interleaveData [interleaverIndex][i] = Data [i];
	   }
	   interleaverIndex = (interleaverIndex + 1) & 0x0F;
//	only continue when de-interleaver is filled
	   if (countforInterleaver <= 15) {
	      countforInterleaver ++;
	      continue;
	   }
//
	   protectionHandler -> deconvolve (tempX, fragmentSize, outV);
//
//	and the inline energy dispersal
	   memset (shiftRegister, 1, 9);
	   for (i = 0; i < bitRate * 24; i ++) {
	      uint8_t b = shiftRegister [8] ^ shiftRegister [4];
	      for (j = 8; j > 0; j--)
	         shiftRegister [j] = shiftRegister [j - 1];
	      shiftRegister [0] = b;
	      outV [i] ^= b;
	   }
	   our_dabProcessor -> addtoFrame (outV);
	}
	fprintf (stderr, "dab-audio handling says goodbye\n");
}
//
//	It might take a msec for the task to stop
void	dabAudio::stopRunning (void) {
	if (running. load ()) {
	   running. store (false);
	   threadHandle. join ();
	}
}

