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
#include	"device-handler.h"
#include	"timesyncer.h"
#include	"dab-api.h"

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
	                                    myReader (this,
	                                              inputDevice,
	                                              spectrumBuffer),
	                                    phaseSynchronizer (&params,
	                                                       threshold,
	                                                       DIFF_LENGTH),
	                                    my_ofdmDecoder (&params,
	                                                    iqBuffer,
	                                                    fic,
	                                                    msc) {
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
        this    -> spectrumBuffer       = spectrumBuffer;
	this	-> my_ficHandler	= fic;

	isSynced	= false;
	running. store (false);
}

	ofdmProcessor::~ofdmProcessor	(void) {
	stop ();
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


/***
   *	\brief run
   *	The main thread, reading samples,
   *	time synchronization and frequency synchronization
   *	Identifying blocks in the DAB frame
   *	and sending them to the ofdmDecoder who will transfer the results
   *	Finally, estimating the small freqency error
   */
void	ofdmProcessor::run	(void) {
std::complex<float>	FreqCorr;
timeSyncer      myTimeSyncer (&myReader);
int32_t		i;
float		fineOffset	= 0;
float		coarseOffset	= 0;
bool		correctionNeeded	= true;
std::complex<float>	ofdmBuffer [T_null];
int		dip_attempts		= 0;
int		index_attempts		= 0;

	my_ofdmDecoder. start ();
	this	-> my_ficHandler -> clearEnsemble ();

	try {
	   sLevel	= 0;
	   for (i = 0; i < T_F / 2; i ++) {
	      jan_abs (myReader. getSample (0));
	   }

//      As long as we are not (time) synced, we try to handle that
//Initing:
notSynced:
           switch (myTimeSyncer. sync (T_null, T_F)) {
              case TIMESYNC_ESTABLISHED:
                 break;                 // yes, we are ready

              case NO_DIP_FOUND:
                 if  (++ dip_attempts >= 5) {
                    syncsignalHandler (false, userData);
                    dip_attempts = 0;
                 }
                 goto notSynced;

              default:                  // does not happen
              case NO_END_OF_DIP_FOUND:
                 goto notSynced;
           }

SyncOnPhase:
//      We arrive here when time synchronized, either from above
//      or after having processed a frame
//      We now have to find the exact first sample of the non-null period.
//      We use a correlation that will find the first sample after the
//      cyclic prefix.
//      Now read in Tu samples. The precise number is not really important
//      as long as we can be sure that the first sample to be identified
//      is part of the samples read.
	   myReader. getSamples (ofdmBuffer,
                                 T_u, coarseOffset + fineOffset);
	   int startIndex = phaseSynchronizer. findIndex (ofdmBuffer);
	   if (startIndex < 0) { // no sync, try again
	      isSynced	= false;
	      if (++index_attempts > 10) {
	         syncsignalHandler (false, userData);
	         index_attempts	= 0;
	      }
	      goto notSynced;
	   }
	   index_attempts	= 0;
	   isSynced	= true;
	   syncsignalHandler (isSynced, userData);

//	Once here, we are synchronized, we need to copy the data we
//	used for synchronization for block 0

	   memmove (ofdmBuffer, &ofdmBuffer [startIndex],
	                  (T_u - startIndex) * sizeof (std::complex<float>));
	   int ofdmBufferIndex	= T_u - startIndex;

//	Block 0 is special in that it is used for coarse time synchronization
//	and its content is used as a reference for decoding the
//	first datablock.
//	We read the missing samples in the ofdm buffer
	   myReader. getSamples (&ofdmBuffer [ofdmBufferIndex],
	                  T_u - ofdmBufferIndex,
	                  coarseOffset + fineOffset);
	   my_ofdmDecoder. processBlock_0 (ofdmBuffer);
//
//	if correction is needed (known by the fic handler)
//	we compute the coarse offset in the phaseSynchronizer
	   correctionNeeded = !my_ficHandler -> syncReached ();
	   if (correctionNeeded) {
	      int correction  = phaseSynchronizer.
	                                  estimateOffset (ofdmBuffer);
	      if (correction != 100) {
	         coarseOffset += correction * carrierDiff;
	         if (abs (coarseOffset) > Khz (35))
	            coarseOffset = 0;
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
	   for (int ofdmSymbolCount = 1;
	        ofdmSymbolCount < (uint16_t)nrBlocks; ofdmSymbolCount ++) {
	      myReader. getSamples (ofdmBuffer, T_s, coarseOffset + fineOffset);
	      for (i = (int)T_u; i < (int)T_s; i ++) 
	         FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
	      my_ofdmDecoder. decodeBlock (ofdmBuffer, ofdmSymbolCount);
	   }

//	we integrate the newly found frequency error with the
//	existing frequency error.
	   fineOffset += 0.1 * arg (FreqCorr) / M_PI * (carrierDiff);

//	at the end of the frame, just skip Tnull samples
	   myReader. getSamples (ofdmBuffer, T_null,
	                                 coarseOffset + fineOffset);
	   if (fineOffset > carrierDiff / 2) {
	      coarseOffset += carrierDiff;
	      fineOffset -= carrierDiff;
	   }
	   else
	   if (fineOffset < - carrierDiff / 2) {
	      coarseOffset -= carrierDiff;
	      fineOffset += carrierDiff;
	   }
	   goto SyncOnPhase;
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

void	ofdmProcessor::show_Corrector (int freqOffset) {
	if (systemdataHandler != NULL)
	   systemdataHandler (isSynced,
	                      my_ofdmDecoder. get_snr (),
	                      freqOffset, userData);
}

bool	ofdmProcessor::signalSeemsGood	(void) {
	return isSynced;
}

