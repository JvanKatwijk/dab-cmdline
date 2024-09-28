#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB-library
 *
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
#include	"audio-backend.h"
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
	audioBackend::audioBackend	(audiodata	*d,
	                                 API_struct	*p,
	                                 void		*ctx):
	                                     virtualBackend (d -> startAddr,
	                                                     d -> length),
	                                     outV (24 * d -> bitRate),
	                                     freeSlots (20) {
int32_t i, j;

	this    -> dabModus             = d -> ASCTy == 077 ? DAB_PLUS : DAB;
	this    -> fragmentSize         = d -> length * CUSize;
	this	-> bitRate		= d -> bitRate;
	this	-> shortForm		= d -> shortForm;
	this	-> protLevel		= d -> protLevel;

	interleaveData		= new int16_t *[16]; // max size
	for (i = 0; i < 16; i ++) {
	   interleaveData [i] = new int16_t [fragmentSize];
	   memset (interleaveData [i], 0, fragmentSize * sizeof (int16_t));
	}

	interleaverIndex	= 0;
	countforInterleaver	= 0;

	if (shortForm)
	   protectionHandler	= new uep_protection (bitRate,
	                                              protLevel);
	else
	   protectionHandler	= new eep_protection (bitRate,
	                                              protLevel);

//	fprintf (stderr, "protection handler is %s\n",
//	                        shortForm ? "uep_protection" : "eep_protection");
	if (dabModus == DAB) 
	   our_backendBase = new mp2Processor (bitRate, p, ctx);
	else
	if (dabModus == DAB_PLUS) 
	   our_backendBase = new mp4Processor (bitRate, p, ctx);
	else		// cannot happen
	   our_backendBase = new backendBase ();

//	fprintf (stderr, "we have now %s\n", dabModus == DAB_PLUS ? "DAB+" : "DAB");
	tempX . resize (fragmentSize);
	nextIn			= 0;
	nextOut			= 0;
	for (i = 0; i < 20; i ++)
	   theData [i] = new int16_t [fragmentSize];

	uint8_t	shiftRegister [9];
	disperseVector. resize (bitRate * 24);
        memset (shiftRegister, 1, 9);
        for (i = 0; i < bitRate * 24; i ++) {
           uint8_t b = shiftRegister [8] ^ shiftRegister [4];
           for (j = 8; j > 0; j--)
              shiftRegister [j] = shiftRegister [j - 1];
           shiftRegister [0] = b;
           disperseVector [i] = b;
	}

	start ();
}

	audioBackend::~audioBackend	() {
int16_t	i;
	if (running. load ()) {
	   running. store (false);
	   threadHandle. join ();
	}
//	delete our_backendBase;
	delete protectionHandler;
	for (i = 0; i < 16; i ++) 
	   delete[]  interleaveData [i];
	delete [] interleaveData;

	for (i = 0; i < 20; i ++)
	   delete [] theData [i];

}

void	audioBackend::start		(void) {
	running. store (true);
	threadHandle = std::thread (&audioBackend::run, this);
}

int32_t	audioBackend::process	(int16_t *v, int16_t cnt) {
	if (!running. load ())
	   return 0;
	while (!freeSlots. tryAcquire (200))
	   if (!running. load ())
	      return 0;
	memcpy (theData [nextIn], v, fragmentSize * sizeof (int16_t));
	nextIn = (nextIn + 1) % 20;
	usedSlots. Release ();
	return 1;
}

const	int16_t interleaveMap [] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
void    audioBackend::processSegment (int16_t *Data) {
int16_t i;

        for (i = 0; i < fragmentSize; i ++) {
           tempX [i] = interleaveData [(interleaverIndex +
                                        interleaveMap [i & 017]) & 017][i];
           interleaveData [interleaverIndex][i] = Data [i];
        }

        interleaverIndex = (interleaverIndex + 1) & 0x0F;
        nextOut = (nextOut + 1) % 20;
        freeSlots. Release ();

//      only continue when de-interleaver is filled
        if (countforInterleaver <= 15) {
           countforInterleaver ++;
           return;
        }

	protectionHandler -> deconvolve (tempX. data (),
	                                 fragmentSize,
	                                  outV. data ());
//
//      and the energy dispersal
	for (i = 0; i < bitRate * 24; i ++)
	   outV [i] ^= disperseVector [i];

	our_backendBase -> addtoFrame (outV. data ());
}

void    audioBackend::run       (void) {

        while (running. load ()) {
           while (!usedSlots. tryAcquire (200))
              if (!running. load ())
                 return;
           processSegment (theData [nextOut]);
        }
}

//
//	It might take a msec for the task to stop
void	audioBackend::stopRunning (void) {
	if (running. load ()) {
	   running. store (false);
	   threadHandle. join ();
	}
}

