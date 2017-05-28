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
 *
 *	Once the bits are "in", interpretation and manipulation
 *	should reconstruct the data blocks.
 *	Ofdm_decoder is called once every Ts samples, and
 *	its invocation results in 2 * Tu bits
 */
#include	"ofdm-decoder.h"
#include	"phasetable.h"
#include	"fic-handler.h"
#include	"msc-handler.h"
#include	"freq-interleaver.h"
#include	"dab-params.h"
/**
  *	\brief ofdmDecoder
  *	The class ofdmDecoder is - when implemented in a separate thread -
  *	taking the data from the ofdmProcessor class in, and
  *	will extract the Tu samples, do an FFT and extract the
  *	carriers and map them on (soft) bits
  */
	ofdmDecoder::ofdmDecoder	(dabParams	*params,
                                         RingBuffer<std::complex<float>> *iqBuffer,
	                                 ficHandler	*my_ficHandler,
	                                 mscHandler	*my_mscHandler):
		                          bufferSpace (params ->  get_L ()),
	                                  myMapper        (params) {
int16_t	i;
	this	-> params		= params;
        this    -> iqBuffer             = iqBuffer;
	this	-> my_ficHandler	= my_ficHandler;
	this	-> my_mscHandler	= my_mscHandler;
	this	-> T_s			= params -> get_T_s ();
	this	-> T_u			= params -> get_T_u ();
	this	-> nrBlocks		= params -> get_L ();
	this	-> carriers		= params -> get_carriers ();
	ibits				= new int16_t [2 * carriers];

	this	-> T_g			= T_s - T_u;
	fft_handler			= new common_fft (T_u);
	fft_buffer			= fft_handler -> getVector ();
	phaseReference			= new std::complex<float> [T_u];
//
	current_snr			= 0;	
	cnt				= 0;

/**
  *	When implemented in a thread, the thread controls the
  *	reading in of the data and processing the data through
  *	functions for handling block 0, FIC blocks and MSC blocks.
  *
  *	We just create a large buffer where index i refers to block i.
  *
  */
	command			= new std::complex<float> * [nrBlocks];
	for (i = 0; i < nrBlocks; i ++)
	   command [i] = new std::complex<float> [T_u];
	amount		= 0;
//
//	will be started from within ofdmProcessor
}

	ofdmDecoder::~ofdmDecoder	(void) {
int16_t	i;
	if (running. load ()) {
	   running. store (false);
//	signal to unlock - if required
	   bufferSpace. notify ();
	   Locker. notify_all ();
	   threadHandle. join ();
	}

	delete		fft_handler;
	delete[]	phaseReference;
	for (i = 0; i < nrBlocks; i ++)
	   delete[] command [i];
	delete[] command;
}

void	ofdmDecoder::start	(void) {
	amount		= 0;
	running. store (true);
	threadHandle	= std::thread (&ofdmDecoder::run, this);
}
	
void	ofdmDecoder::stop		(void) {
	if (running. load ()) {
	   running. store (false);
	   Locker. notify_all ();
	   amount	= 100;
	   threadHandle. join ();
	}
}
/**
  *	The code in the thread executes a simple loop,
  *	waiting for the next block and executing the interpretation
  *	operation for that block.
  *	In our original code the block count was 1 higher than
  *	our count here.
  */
void	ofdmDecoder::run	(void) {
int16_t	currentBlock	= 0;
std::unique_lock<std::mutex> lck (ourMutex);
//
//	we will ensure that a signal is sent on task termination
	while (running. load ()) {
	   while ((amount <= 0) && running. load ()) {
	      auto now = std::chrono::system_clock::now ();
	      Locker. wait_until (lck, now + std::chrono::milliseconds (1));
	   }
	      
	   while ((amount > 0) && running. load ()) {
	      if (currentBlock == 0)
	         processBlock_0 ();
	      else
	      if (currentBlock < 4)
	         decodeFICblock (currentBlock);
	      else
	         decodeMscblock (currentBlock);
	      currentBlock = (currentBlock + 1) % nrBlocks;
	      myMutex. lock ();
	      amount -= 1;
	      myMutex. unlock ();
	      bufferSpace. notify ();
	   }
	}
}
/**
  *	We need some functions to enter the ofdmProcessor data
  *	in the buffer.
  */
void	ofdmDecoder::processBlock_0 (std::complex<float> *vi) {
	bufferSpace. wait ();
	memcpy (command [0], vi, sizeof (std::complex<float>) * T_u);
	myMutex. lock ();
	amount ++;
	myMutex. unlock ();
	Locker. notify_one ();
}

void	ofdmDecoder::decodeFICblock (std::complex<float> *vi, int32_t blkno) {
	bufferSpace. wait ();
	memcpy (command [blkno], &vi [T_g], sizeof (std::complex<float>) * T_u);
	myMutex. lock ();
	amount ++;
	myMutex. unlock ();
	Locker. notify_one ();
}

void	ofdmDecoder::decodeMscblock (std::complex<float> *vi, int32_t blkno) {
	bufferSpace. wait ();
	memcpy (command [blkno], &vi [T_g], sizeof (std::complex<float>) * T_u);
	myMutex. lock ();
	amount ++;
	myMutex. unlock ();
	Locker. notify_one ();
}
/**
  *	Note that the distinction, made in the ofdmProcessor class
  *	does not add much here, iff we decide to choose the multi core
  *	option definitely, then code may be simplified there.
  */
void	ofdmDecoder::processBlock_0 (void) {

	memcpy (fft_buffer, command [0], T_u * sizeof (std::complex<float>));
	fft_handler	-> do_FFT ();
/**
  *	The SNR is determined by looking at a segment of bins
  *	within the signal region and bits outside.
  *	It is just an indication
  */
	current_snr	= 0.7 * current_snr + 0.3 * get_snr (fft_buffer);
/**
  *	we are now in the frequency domain, and we keep the carriers
  *	as coming from the FFT as phase reference.
  */
	memcpy (phaseReference, fft_buffer, T_u * sizeof (std::complex<float>));
}
/**
  *	for the other blocks of data, the first step is to go from
  *	time to frequency domain, to get the carriers.
  *	we distinguish between FIC blocks and other blocks,
  *	only to spare a test. The mapping code is the same
  *
  *	\brief decodeFICblock
  *	do the transforms and hand over the result to the fichandler
  */
void	ofdmDecoder::decodeFICblock (int32_t blkno) {
int16_t	i;
std::complex<float> conjVector [T_u];

	memcpy (fft_buffer, command [blkno], T_u * sizeof (std::complex<float>));
fftlabel:
/**
  *	first step: do the FFT
  */
	fft_handler -> do_FFT ();
/**
  *	a little optimization: we do not interchange the
  *	positive/negative frequencies to their right positions.
  *	The de-interleaving understands this
  */
toBitsLabel:
/**
  *	Note that from here on, we are only interested in the
  *	"carriers" useful carriers of the FFT output
  */
	for (i = 0; i < carriers; i ++) {
	   int16_t	index	= myMapper. mapIn (i);
	   if (index < 0) 
	      index += T_u;
/**
  *	decoding is computing the phase difference between
  *	carriers with the same index in subsequent blocks.
  *	The carrier of a block is the reference for the carrier
  *	on the same position in the next block
  */
	   std::complex<float>	r1 = fft_buffer [index] * conj (phaseReference [index]);
	   phaseReference [index] = fft_buffer [index];
           conjVector [index] = r1;
	   float ab1	= jan_abs (r1);
//	The viterbi decoder expects values in the range 0 .. 255,
//	we present values -127 .. 127 (easy with depuncturing)
	   ibits [i]		= - real (r1) / ab1 * 127.0;
	   ibits [carriers + i] = - imag (r1) / ab1 * 127.0;
	}
handlerLabel:
	my_ficHandler -> process_ficBlock (ibits, blkno);

//	From time to time we show the constellation of block 2.
//	Note that we do it in two steps since the
//	fftbuffer contained low and high at the ends
//	and we maintain that format
	if ((blkno == 2) && (iqBuffer != NULL)) {
	   if (++cnt > 7) {
	      iqBuffer	-> putDataIntoBuffer (&conjVector [0],
	                                      carriers / 2);
	      iqBuffer	-> putDataIntoBuffer (&conjVector [T_u - 1 - carriers / 2],
	                                      carriers / 2);
//	      iqBuffer	-> putDataIntoBuffer (&fft_buffer [0],
//	                                      carriers / 2);
//	      iqBuffer	-> putDataIntoBuffer (&fft_buffer [T_u - 1 - carriers / 2],
//	                                      carriers / 2);
	      cnt = 0;
	   }
	}
}
/**
  *	Msc block decoding is equal to FIC block decoding, but we
  *	restrain the blocks from which the constellation is shown
  *	to the FIC blocks
  */
void	ofdmDecoder::decodeMscblock (int32_t blkno) {
int16_t	i;

	memcpy (fft_buffer, command [blkno], T_u * sizeof (std::complex<float>));
fftLabel:
	fft_handler -> do_FFT ();
//
//	Note that "mapIn" maps to -carriers / 2 .. carriers / 2
//	we did not set the fft output to low .. high
toBitsLabel:
	for (i = 0; i < carriers; i ++) {
	   int16_t	index	= myMapper. mapIn (i);
	   if (index < 0) 
	      index += T_u;
	      
	   std::complex<float>	r1 = fft_buffer [index] * conj (phaseReference [index]);
	   phaseReference [index] = fft_buffer [index];
	   float ab1	= jan_abs (r1);
//
//	The viterbi decoder expects values in the range 0 .. 255
	   ibits [i]		= - real (r1) / ab1 * 127.0;
	   ibits [carriers + i] = - imag (r1) / ab1 * 127.0;
	}
handlerLabel:
	my_mscHandler -> process_mscBlock (ibits, blkno);
}

//
//
/**
  *	for the snr we have a full T_u wide vector, with in the middle
  *	K carriers.
  *	Just get the strength from the selected carriers compared
  *	to the strength of the carriers outside that region
  */
int16_t	ofdmDecoder::get_snr (std::complex<float> *v) {
int16_t	i;
float	noise 	= 0;
float	signal	= 0;
int16_t	low	= T_u / 2 -  carriers / 2;
int16_t	high	= low + carriers;

	for (i = 10; i < low - 20; i ++)
	   noise += abs (v [(T_u / 2 + i) % T_u]);

	for (i = high + 20; i < T_u - 10; i ++)
	   noise += abs (v [(T_u / 2 + i) % T_u]);

	noise	/= (low - 30 + T_u - high - 30);
	for (i = T_u / 2 - carriers / 4;  i < T_u / 2 + carriers / 4; i ++)
	   signal += abs (v [(T_u / 2 + i) % T_u]);
	return get_db (signal / (carriers / 2)) - get_db (noise);
}

int16_t	ofdmDecoder::get_snr	(void) {
	return (int16_t)current_snr;
}

