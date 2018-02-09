#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
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
 *    along with dab library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#
#include	"dab-constants.h"
#include	"data-backend.h"
#include	"dab-processor.h"
#include	"eep-protection.h"
#include	"uep-protection.h"
#include	"data-processor.h"
#include	<chrono>

//
//	fragmentsize == Length * CUSize
	dataBackend::dataBackend	(packetdata	*d,
	                                 bytesOut_t	bytesOut,
	                                 motdata_t	motdataHandler,
	                                 void	*ctx) :
                                         virtualBackend (d -> startAddr,
                                                         d -> length),
	                                 outV (24 * d -> bitRate),
	                                 freeSlots (20) {
int32_t i, j;
        this    -> packetAddress        = d -> packetAddress;
        this    -> fragmentSize         = d -> length * CUSize;
        this    -> bitRate              = d -> bitRate;
        this    -> shortForm            = d -> shortForm;
        this    -> protLevel            = d -> protLevel;
        our_dabProcessor        = new dataProcessor (bitRate,
                                                     d -> DSCTy,
                                                     d -> appType,
                                                     d -> DGflag,
                                                     d -> FEC_scheme,
                                                     bytesOut,
	                                             motdataHandler,
                                                     ctx);
	nextIn		= 0;
	nextOut		= 0;
	for (i = 0; i < 20; i ++)
	   theData [i] = new int16_t [fragmentSize];

	tempX. resize (fragmentSize);
	interleaveData		= new int16_t *[16]; // the size
	for (i = 0; i < 16; i ++) {
	   interleaveData [i] = new int16_t [fragmentSize];
	   memset (interleaveData [i], 0, fragmentSize * sizeof (int16_t));
	}
	countforInterleaver	= 0;
//
//	The handling of the depuncturing and deconvolution is
//	shared with that of the audio
	if (shortForm)
	   protectionHandler	= new uep_protection (bitRate,
	                                              protLevel);
	else
	   protectionHandler	= new eep_protection (bitRate,
	                                              protLevel);
//
//	any reasonable (i.e. large) size will do here,
//	as long as the parameter is a power of 2
	uint8_t shiftRegister [9];
	disperseVector. resize (24 * bitRate);
	memset (shiftRegister, 1, 9);
	for (i = 0; i < bitRate * 24; i ++) {
	   uint8_t b = shiftRegister [8] ^ shiftRegister [4];
	   for (j = 8; j > 0; j--)
	      shiftRegister [j] = shiftRegister [j - 1];
	   shiftRegister [0] = b;
	   disperseVector [i] = b;
	}
	running. store (false);
	start ();
}

	dataBackend::~dataBackend (void) {
int16_t	i;
	if (running. load ()) {
	   threadHandle. join ();
	   running. store (false);
	}

	delete protectionHandler;
	for (i = 0; i < 16; i ++)
	   delete[] interleaveData [i];
	delete[]	interleaveData;
	for (i = 0; i < 20; i ++)
	   delete [] theData [i];
}

void    dataBackend::start         (void) {
        threadHandle = std::thread (&dataBackend::run, this);
}

int32_t	dataBackend::process	(int16_t *v, int16_t cnt) {
	(void)cnt;
	while (!freeSlots. tryAcquire (200))
	   if (!running)
	      return 0;
	memcpy (theData [nextIn], v, fragmentSize * sizeof (int16_t));
	nextIn = (nextIn + 1) % 20;
	usedSlots. Release ();
	return 1;
}

const   int16_t interleaveMap[] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};

void	dataBackend::run	(void) {
int16_t	i;
int16_t	countforInterleaver	= 0;
int16_t interleaverIndex	= 0;
int16_t	Data [fragmentSize];

	running. store (true);
	while (running. load ()) {
	   while (!usedSlots. tryAcquire (200))
	      if (!running)
	         return;

	   for (i = 0; i < fragmentSize; i ++) {
	      tempX [i] = interleaveData [(interleaverIndex +
	                          interleaveMap [i & 017]) & 017][i];
	      interleaveData [interleaverIndex][i] = theData [nextOut] [i];
	   }
	   nextOut = (nextOut + 1) % 20;
	   freeSlots. Release ();

	   interleaverIndex = (interleaverIndex + 1) & 0x0F;

//	only continue when de-interleaver is filled
	   if (countforInterleaver <= 15) {
	      countforInterleaver ++;
	      continue;
	   }
//
	   protectionHandler -> deconvolve (tempX. data (),
	                                       fragmentSize, outV. data ());
//
//	and the energy dispersal
	   for (i = 0; i < bitRate * 24; i ++)
	      outV [i] ^= disperseVector [i];
//	What we get here is a long sequence (24 * bitrate) of bits, not packed
//	but forming a DAB packet
//	we hand it over to make an MSC data group
	   our_dabProcessor -> addtoFrame (outV. data ());
	}
}

//	It might take a msec for the task to stop
void	dataBackend::stopRunning (void) {
	running. store (false);
	threadHandle. join ();
}

