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
#include	"msc-handler.h"
#include	"audio-backend.h"
#include	"data-backend.h"
#include	"dab-params.h"
//
//	Interface program for processing the MSC.
//	Merely a dispatcher for the selected service
//
//	The ofdm processor assumes the existence of an msc-handler, whether
//	a service is selected or not. 

#define	CUSize	(4 * 16)
//	Note CIF counts from 0 .. 3

static
int16_t	cifVector [55296];

static int blocksperCIF [] = {18, 72, 0, 36};

		mscHandler::mscHandler	(uint8_t	dabMode,
	                                 audioOut_t	soundOut,
	                                 dataOut_t	dataOut,
	                                 bytesOut_t	bytesOut,
	                                 programQuality_t mscQuality,
	                                 motdata_t	motdata_Handler,
	                                 void		*userData):
	                                    params (dabMode),
	                                    my_fftHandler (dabMode),
	                                    myMapper (dabMode),
	                                    freeSlots (params. get_L ()) {
	this	-> soundOut		= soundOut;
	this	-> dataOut		= dataOut;
	this	-> bytesOut		= bytesOut;
	this	-> programQuality	= mscQuality;
	this	-> motdata_Handler	= motdata_Handler;
	this	-> userData		= userData;
	theData				= new std::complex<float> *[params. get_L ()];
	for (int i = 0; i < params. get_L (); i ++)
	   theData [i] = new std::complex<float> [params. get_T_u ()];

//	cifVector. resize (55296);
	cifCount		= 0;	// msc blocks in CIF
	theBackends. push_back (new virtualBackend (0, 0));
	BitsperBlock		= 2 * params. get_carriers ();
	numberofblocksperCIF	= blocksperCIF [(dabMode - 1) & 03];

	work_to_do. store (false);
	running. store (false);
	phaseReference. resize (params. get_T_u ());
}

	mscHandler::~mscHandler	(void) {
	stop ();
	for (int i = 0; i < params. get_L (); i ++)
	   delete [] theData [i];
	delete [] theData;
}

void	mscHandler::stop (void) {
	if (running. load ()) {
	   running. store (false);
	   threadHandle. join ();
	}

	mutexer. lock ();
	for (auto const &b : theBackends) {
	   b -> stopRunning ();
	   delete b;
	}

	theBackends. resize (0);
	work_to_do. store (false);
	mutexer. unlock ();
}

void	mscHandler::start	(void) {
	if (running.load ()) {
	   fprintf (stderr, "cannot restart mscHandler, still active\n");
	   return;
	}
//	else
//	   fprintf (stderr, "starting mscHandler\n");
	threadHandle = std::thread (&mscHandler::run, this);
}

void	mscHandler::reset	(void) {
	stop ();
	start ();
}

//	The exteral world sees this
void    mscHandler::process_mscBlock (std::complex<float> *b, int16_t blkno) {
	while (running. load ())
	   if (freeSlots. tryAcquire (200))
	      break;

	if (!running. load ())
	   return;
	memcpy (theData [blkno], b,
	            params. get_T_u () * sizeof (std::complex<float>));
	usedSlots. Release ();
}

void	mscHandler::run       (void) {
int	currentBlock	= 0;
std::vector<int16_t> ibits;

	running. store (true);
	fft_buffer	= my_fftHandler. getVector ();
	ibits. resize (BitsperBlock);
	while (running. load ()) {
	   while (!usedSlots. tryAcquire (200))
	      if (!running. load ())
	         return;
	   memcpy (fft_buffer, theData [currentBlock],
	                 params. get_T_u () * sizeof (std::complex<float>));

//      block 3 and up are needed as basis for demodulation the "mext" block
//      "our" msc blocks start with blkno 4
	   my_fftHandler. do_FFT ();
	   if (currentBlock >= 4) {
	      for (int i = 0; i < params. get_carriers (); i ++) {
	         int16_t      index   = myMapper. mapIn (i);
	         if (index < 0)
	            index += params. get_T_u ();

	         std::complex<float>  r1 = fft_buffer [index] *
	                               conj (phaseReference [index]);
	         float ab1    = jan_abs (r1);
//	Recall:  the viterbi decoder wants 127 max pos, - 127 max neg
//	we make the bits into softbits in the range -127 .. 127
	         ibits [i]            =  - real (r1) / ab1 * 127.0;
	         ibits [params. get_carriers () + i]
	                                 =  - imag (r1) / ab1 * 127.0;
	      }
	      process_mscBlock (ibits, currentBlock);
	   }

	   memcpy (phaseReference. data (), fft_buffer,
	                 params. get_T_u () * sizeof (std::complex<float>));
	   freeSlots. Release ();
	   currentBlock = (currentBlock + 1) % (params. get_L ());
	}
}

//
//	Note, the set_xxx functions are called from within a
//	different thread than the process_mscBlock method,
//	so, a little bit of locking seems wise while
//	the actual changing of the settings is done in the
//	thread executing process_mscBlock
void	mscHandler::set_audioChannel (audiodata *d) {
	mutexer. lock ();
//
//	we could assert here that theBackend == nullptr
	theBackends. push_back (new audioBackend (d,
	                                    soundOut,
	                                    dataOut,
	                                    programQuality,
	                                    motdata_Handler,
	                                    userData));
	work_to_do. store (true);
	mutexer. unlock ();
}


void	mscHandler::set_dataChannel (packetdata *d) {
	mutexer. lock ();
	theBackends. push_back (new dataBackend (d,
	                                   bytesOut,
	                                   motdata_Handler,
	                                   userData));
	work_to_do. store (true);
	mutexer. unlock ();
}

void	mscHandler::process_mscBlock	(std::vector<int16_t> fbits,
	                                 int16_t blkno) { 
int16_t	currentblk;

//	we accept the incoming data
	currentblk	= (blkno - 4) % numberofblocksperCIF;
	memcpy (&cifVector [currentblk * BitsperBlock],
	                    fbits. data (), BitsperBlock * sizeof (int16_t));
	if (currentblk < numberofblocksperCIF - 1) 
	   return;

	if (!work_to_do. load ())
	   return;
//	OK, now we have a full CIF
	mutexer. lock ();
	cifCount	= (cifCount + 1) & 03;
	for (auto const& b: theBackends) {
	   int startAddr	= b -> startAddr ();
	   int Length		= b -> Length    ();

	   if (Length > 0) {
	      int16_t myBegin [Length * CUSize];
	      memcpy (myBegin, &cifVector [startAddr * CUSize],
	                               Length * CUSize * sizeof (int16_t));
	      (void) b -> process (myBegin, Length * CUSize);
	   }
	}
	mutexer. unlock ();
}

