#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the  DAB-library
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
#include	"ofdm-processor.h"
#include	"ofdm-decoder.h"
#include	"msc-handler.h"
#include	"fic-handler.h"
#include	"dab-api.h"
#include	"device-handler.h"
#include	"fft_handler.h"

/**
  *	\brief ofdmProcessor
  *	The ofdmProcessor class is the driver of the processing
  *	of the samplestream.
  *	It takes as parameter (a.o) the handler for the
  *	input device as well as the interpreters for
  *	FIC blocks and for MSC blocks.
  *	Local is a class ofdmDecoder that will - as the name suggests -
  *	map samples to bits and that will pass on the bits
  *	to the interpreters for FIC and MSC
  */

	ofdmProcessor::ofdmProcessor	(deviceHandler	*inputDevice,
	                                 uint8_t	Mode,
	                                 syncsignal_t	syncsignalHandler,
	                                 systemdata_t	systemdataHandler,
	                                 mscHandler 	*msc,
	                                 ficHandler 	*fic,
	                                 int16_t	threshold,
	                                 RingBuffer<std::complex<float>> *spectrumBuffer,
	                                 RingBuffer<std::complex<float>> *iqBuffer,
	                                 void		*userData):
	                                    params (Mode),
	                                    my_fftHandler (Mode),
	                                    phaseSynchronizer (&params,
	                                                       &my_fftHandler,
	                                                       threshold,
	                                                       DIFF_LENGTH),
	                                    my_ofdmDecoder (&params,
	                                                    &my_fftHandler,
	                                                    iqBuffer,
	                                                    fic,
	                                                    msc) {
int32_t	i;
	this	-> inputDevice		= inputDevice;
	this	-> syncsignalHandler	= syncsignalHandler;
	this	-> systemdataHandler	= systemdataHandler;
	this	-> userData		= userData;
	this	-> T_null		= params. get_T_null ();
	this	-> T_s			= params. get_T_s ();
	this	-> T_u			= params. get_T_u ();
	this	-> T_F			= params. get_T_F ();
	this	-> nrBlocks		= params. get_L ();
	this	-> carriers		= params. get_carriers ();
	this	-> carrierDiff		= params. get_carrierDiff ();
        bufferSize			= 32768;
        this    -> spectrumBuffer       = spectrumBuffer;
        localBuffer. resize (bufferSize);
        localCounter			= 0;
	this	-> my_ficHandler	= fic;
	tiiSwitch			= false;
//
	sampleCnt			= 0;

/**
  *	the class phaseReference will take a number of samples
  *	and indicate - using some threshold - whether there is
  *	a strong correlation or not.
  *	It is used to decide on the first non-null sample
  *	of the frame.
  *	The size of the blocks handed over for inspection
  *	is T_u
  */
//	fineCorrector		= 0;	
//	coarseCorrector		= 0;
//	correctionNeeded	= true;
	oscillatorTable		= new std::complex<float> [INPUT_RATE];

	for (i = 0; i < INPUT_RATE; i ++)
	   oscillatorTable [i] =
	           std::complex<float> (cos (2.0 * M_PI * i / INPUT_RATE),
	                                sin (2.0 * M_PI * i / INPUT_RATE));

	bufferContent	= 0;
	isSynced	= false;
	running. store (false);
}

	ofdmProcessor::~ofdmProcessor	(void) {
	stop ();
	delete		oscillatorTable;
}

void	ofdmProcessor::start	(void) {

	if (running. load ())
	   return;
	sLevel		= 0;
	isSynced	= false;
	localPhase	= 0;
	running. store (true);
	threadHandle	= std::thread (&ofdmProcessor::run, this);
}

//	Profiling shows that getting a sample, together
//	with the frequency shift, is a real performance killer.
//	we therefore distinguish between getting a single sample
//	and getting a vector full of samples

std::complex<float> ofdmProcessor::getSample (int32_t freqOffset) {
std::complex<float> temp;

	if (!running. load ()) 
	   throw 21;

	if (bufferContent == 0) {
	   bufferContent = inputDevice -> Samples ();
	   while (running. load () && (bufferContent == 0)) {
	      usleep (1000);
	      bufferContent = inputDevice -> Samples (); 
	   }
	}

	if (!running. load ())	
	   throw 20;
//
//	so here, bufferContent > 0, fetch a sample
	inputDevice -> getSamples (&temp, 1);
	bufferContent --;
        if (localCounter < bufferSize)
           localBuffer [localCounter ++]        = temp;
//	OK, we have a sample!!
//	first: adjust frequency. We need Hz accuracy
	localPhase	-= freqOffset;
	localPhase	= (localPhase + INPUT_RATE) % INPUT_RATE;
	temp		*= oscillatorTable [localPhase];
	sLevel		= 0.00001 * jan_abs (temp) + (1 - 0.00001) * sLevel;
#define	N	5
	sampleCnt	++;
	if (++ sampleCnt > INPUT_RATE / N) {
	   if (spectrumBuffer != NULL) 
              spectrumBuffer -> putDataIntoBuffer (localBuffer. data (),
	                                              localCounter);
           localCounter = 0;
	   call_systemData (isSynced,
	                    my_ofdmDecoder. get_snr (),
	                    freqOffset);
	   sampleCnt = 0;

	}
	return temp;
}
//

void	ofdmProcessor::getSamples (std::complex<float> *v,
	                           int16_t n, int32_t freqOffset) {
int32_t		i;

	if (!running. load ())
	   throw 21;
	if (n > bufferContent) {
	   bufferContent = inputDevice -> Samples ();
	   while (running. load () && (bufferContent < n)) {
	      usleep (1000);
	      bufferContent = inputDevice -> Samples ();
	   }
	}
	if (!running. load ())	
	   throw 20;
//	so here, bufferContent >= n
	n	= inputDevice -> getSamples (v, n);
	bufferContent -= n;

//	OK, we have samples!!
//	first: adjust frequency. We need Hz accuracy
	for (i = 0; i < n; i ++) {
	   localPhase	-= freqOffset;
	   localPhase	= (localPhase + INPUT_RATE) % INPUT_RATE;
           if (localCounter < bufferSize)
              localBuffer [localCounter ++]     = v [i];
	   v [i]	*= oscillatorTable [localPhase];
	   sLevel	= 0.00001 * jan_abs (v [i]) + (1 - 0.00001) * sLevel;
	}

	sampleCnt	+= n;
	if (sampleCnt > INPUT_RATE / N) {
	   if (spectrumBuffer != NULL)
              spectrumBuffer -> putDataIntoBuffer (localBuffer. data (),
	                                                      bufferSize);
           localCounter = 0;
	   call_systemData (isSynced,
	                    my_ofdmDecoder. get_snr (),
	                    freqOffset);
	   sampleCnt = 0;
	}
}

/***
   *	\brief run
   *	The main thread, reading samples,
   *	time synchronization and frequency synchronization
   *	Identifying blocks in the DAB frame
   *	and sending them to the ofdmDecoder who will transfer the results
   *	Finally, estimating the small freqency error
   */
void	ofdmProcessor::run	(void) {
float		fineCorrector	= 0;
float		coarseCorrector	= 0;
float		cLevel		= 0;
int32_t		syncBufferIndex	= 0;
bool		correctionNeeded	= true;
int32_t		startIndex;
int32_t		i;
std::complex<float>	FreqCorr;
int32_t		counter;
int32_t		syncBufferSize	= 32768;
int32_t		syncBufferMask	= syncBufferSize - 1;
float		envBuffer	[syncBufferSize];
std::complex<float>	ofdmBuffer [T_s];
int		ofdmSymbolCount		= 0;
int		dip_attempts		= 0;
int		index_attempts		= 0;

	my_ofdmDecoder. start ();
	this	-> my_ficHandler -> clearEnsemble ();

	try {
	   sLevel	= 0;
	   for (i = 0; i < T_F / 2; i ++) {
	      jan_abs (getSample (0));
	   }

	   while (true) {
//      As long as we are not (time) synced, we try to handle that
              if (!isSynced) {
                 syncBufferIndex        = 0;
                 cLevel		        = 0;
                 for (i = 0; i < 50; i ++) {
                    complex<float> sample       = getSample (0);
                    envBuffer [syncBufferIndex] = jan_abs (sample);
                    cLevel                     += envBuffer [syncBufferIndex];
                    syncBufferIndex ++;
                 }

//	We now have initial values for cLevel (i.e. the sum
//	over the last 50 samples) and sLevel, the long term average.
//	here we start looking for the null level, i.e. a dip
	         counter	= 0;
	         while (cLevel / 50  > 0.40 * sLevel) {
	            std::complex<float> sample	=
	                      getSample (coarseCorrector + fineCorrector);
	            envBuffer [syncBufferIndex] = jan_abs (sample);
	            cLevel += envBuffer [syncBufferIndex] -
	                      envBuffer [(syncBufferIndex - 50) & syncBufferMask];
	            syncBufferIndex = (syncBufferIndex + 1) & syncBufferMask;
	            counter ++;
	
	            if (counter > 2 * T_F) { // hopeless	
	               break;
	            }
	         }
//	if we have 5 successive attempts are failing, signal our bosses
	         if (counter > 2 * T_F) {
                    if (++dip_attempts > 10) {
	               dip_attempts = 0;
//	               syncsignalHandler (false, userData);
	            }
	            continue;
	         }
              }
//	It seemed we found a dip that started app 65/100 * 50 samples earlier.
//	We now start looking for the end of the null period.

	      counter	= 0;
	      while (cLevel / 50 < 0.75 * sLevel) {
	         std::complex<float> sample =
	                    getSample (coarseCorrector + fineCorrector);
	         envBuffer [syncBufferIndex] = jan_abs (sample);
	         cLevel += envBuffer [syncBufferIndex] -
	                   envBuffer [(syncBufferIndex - 50) & syncBufferMask];
	         syncBufferIndex = (syncBufferIndex + 1) & syncBufferMask;
	         counter	++;
	         if (counter > T_null + 50)  // hopeless
	            break;
	      }
	      if (counter > T_null + 50) {
	         continue;
	      }
	      dip_attempts	= 0;
//      We arrive here when time synchronized, either from above
//      or after having processed a frame
//      We now have to find the exact first sample of the non-null period.
//      We use a correlation that will find the first sample after the
//      cyclic prefix.
//      Now read in Tu samples. The precise number is not really important
//      as long as we can be sure that the first sample to be identified
//      is part of the samples read.
	      getSamples (ofdmBuffer,
                                T_u, coarseCorrector + fineCorrector);
	      startIndex = phaseSynchronizer. findIndex (ofdmBuffer);
	      if (startIndex < 0) { // no sync, try again
	         isSynced	= false;
	         if (++index_attempts > 10) {
	            syncsignalHandler (false, userData);
	         }
	         continue;
	      }
	      index_attempts	= 0;
	      syncsignalHandler (true, userData);
	      isSynced	= true;
//	Once here, we are synchronized, we need to copy the data we
//	used for synchronization for block 0

	      memmove (ofdmBuffer, &ofdmBuffer [startIndex],
	                  (T_u - startIndex) * sizeof (std::complex<float>));
	      int ofdmBufferIndex	= T_u - startIndex;

//	Block 0 is special in that it is used for coarse time synchronization
//	and its content is used as a reference for decoding the
//	first datablock.
//	We read the missing samples in the ofdm buffer
	      getSamples (&ofdmBuffer [ofdmBufferIndex],
	                  T_u - ofdmBufferIndex,
	                  coarseCorrector + fineCorrector);
	      my_ofdmDecoder. processBlock_0 (ofdmBuffer);
//
//	Here we look only at the block_0 when we need a coarse
//	frequency synchronization.
//	The width is limited to 2 * 35 Khz (i.e. positive and negative)
	      correctionNeeded = !my_ficHandler -> syncReached ();
	      if (correctionNeeded) {
	         int correction  = phaseSynchronizer.
	                                  estimateOffset (ofdmBuffer);
	         if (correction != 100) {
	            coarseCorrector += correction * carrierDiff;
	            if (abs (coarseCorrector) > Khz (35))
	               coarseCorrector = 0;
	         }
	      }
//
//	after block 0, we will just read in the other (params -> L - 1) blocks
//	The first ones are the FIC blocks. We immediately
//	start with building up an average of the phase difference
//	between the samples in the cyclic prefix and the
//	corresponding samples in the datapart.
///	and similar for the (params. L - 4) MSC blocks
	      FreqCorr		= std::complex<float> (0, 0);
	      for (ofdmSymbolCount = 1;
	           ofdmSymbolCount < (uint16_t)nrBlocks; ofdmSymbolCount ++) {
	         getSamples (ofdmBuffer, T_s, coarseCorrector + fineCorrector);
	         for (i = (int)T_u; i < (int)T_s; i ++) 
	            FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
	
	         my_ofdmDecoder. decodeBlock (ofdmBuffer, ofdmSymbolCount);
	      }

//	we integrate the newly found frequency error with the
//	existing frequency error.
	      fineCorrector += 0.1 * arg (FreqCorr) / M_PI * (carrierDiff);

//	at the end of the frame, just skip Tnull samples
	      getSamples (ofdmBuffer, T_null, coarseCorrector + fineCorrector);
	      counter	= 0;
	      if (fineCorrector > carrierDiff / 2) {
	         coarseCorrector += carrierDiff;
	         fineCorrector -= carrierDiff;
	      }
	      else
	      if (fineCorrector < - carrierDiff / 2) {
	         coarseCorrector -= carrierDiff;
	         fineCorrector += carrierDiff;
	      }
	   }
	}
	catch (int e) {
	   ;
	}
	my_ofdmDecoder. stop ();
	fprintf (stderr, "ofdmProcessor is shutting down\n");
}

void	ofdmProcessor:: reset	(void) {
	if (running. load ()) {
	   running. store (false);
	   sleep (1);
	   threadHandle. join ();
	}
	start ();
}

void	ofdmProcessor::stop	(void) {	
	if (running. load ()) {
	   running. store (false);
	   sleep (1);
	   threadHandle. join ();
	}
}

void	ofdmProcessor::call_systemData (bool f, int16_t snr, int32_t freq) {
	if (systemdataHandler != NULL)
	   systemdataHandler (f, snr, freq, userData);
}

bool	ofdmProcessor::signalSeemsGood	(void) {
	return isSynced;
}

