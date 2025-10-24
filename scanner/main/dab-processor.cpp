#
/*
 *    Copyright (C) 2025
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the 2-nd DAB scannner and borrowed
 *    for the Qt-DAB repository of the same author
 *
 *    DAB scannner is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB scanner is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB scanner; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"dab-processor.h"
#include	"device-handler.h"
#include	"timesyncer.h"
#include	"dab-api.h"

/**
  *	\brief dabProcessor
  *	The dabProcessor class is the driver of the processing
  *	of the samplestream.
  */

#define	THRESHOLD	3
	dabProcessor::dabProcessor	(deviceHandler	*inputDevice,
	                                 API_struct	*p,
	                                 void		*userData):
	                                    theTable	(),
	                                    myReader (this,
	                                              inputDevice),
	                                    myCorrelator (&theTable),
	                                    myFreqSyncer (&theTable),
	                                    my_TII_Detector (),
	                                    my_ofdmDecoder (),
	                                    my_ficHandler (p, userData) {
	this	-> inputDevice		= inputDevice;
	this	-> syncsignalHandler	= p -> syncsignal_Handler;
	this	-> show_tii		= p -> tii_data_Handler;
	this	-> userData		= userData;
	this	-> T_null		= params. get_T_null ();
	this	-> T_s			= params. get_T_s ();
	this	-> T_u			= params. get_T_u ();
	this	-> T_g			= params. get_T_g();
	this	-> T_F			= params. get_T_F ();
	this	-> nrBlocks		= params. get_L ();
	this	-> carriers		= params. get_carriers ();
	this	-> carrierDiff		= params. get_carrierDiff ();
	this	-> tii_counter		= 0;
	this	-> threshold		= p -> thresholdValue;
	isSynced			= false;
	snr				= 0;
	running. store (false);
}

	dabProcessor::~dabProcessor	() {
	stop ();
}

void	dabProcessor::start		() {
	
	if (running. load ())
	   return;
	running. store (true);
	threadHandle	= std::thread (&dabProcessor::run, this);
}

/***
   *	\brief run
   *	The main thread, reading samples,
   *	time synchronization and frequency synchronization
   *	Identifying blocks in the DAB frame
   *	and sending them to the ofdmDecoder who will transfer the results
   *	Finally, estimating the small freqency error
   */
void	dabProcessor::run		() {
std::complex<float>	FreqCorr;
timeSyncer      myTimeSyncer (&myReader);
float		fineOffset		= 0;
float		coarseOffset		= 0;
bool		correctionNeeded	= true;
std::vector<complex<float>>	ofdmBuffer (T_null);
int		index_attempts		= 0;
int		startIndex		= -1;

	isSynced	= false;
	snr		= 10;		// no zero, it is used elsewhere
	running. store (true);
	my_ficHandler. reset ();
	myReader. setRunning (true);

	try {
	   myReader. reset ();
	   for (int i = 0; i < T_F / 2; i ++) {
	     (void)jan_abs (myReader. getSample (0));
	   }

notSynced:
//Initing:
	   my_TII_Detector. reset ();
	   switch (myTimeSyncer. sync (T_null, T_F)) {
	      case TIMESYNC_ESTABLISHED:
	         break;                 // yes, we are ready

	      case NO_DIP_FOUND:
//	         if  (++ dip_attempts >= 5) {
//	            syncsignalHandler (false, userData);
//	            dip_attempts = 0;
//	         }
	         goto notSynced;

	      default:                  // does not happen
	      case NO_END_OF_DIP_FOUND:
	         goto notSynced;
	   }

	   myReader. getSamples (ofdmBuffer. data (),
	                         T_u, coarseOffset + fineOffset);

	   startIndex = myCorrelator.
	                        findIndex (ofdmBuffer, THRESHOLD);
	   if (startIndex < 0) { // no sync, try again
	      isSynced	= false;
	      if (++index_attempts > 25) {
	         fprintf (stderr, "startIndex %d\n", startIndex);
	         index_attempts	= 0;
	      }
	      goto notSynced;
	   }
	   index_attempts	= 0;
	   goto SyncOnPhase;

Check_endofNull:
//	when we are here, we had a (more or less) decent frame,
//	and we are ready for the new one.
//	we just check that we are indeed around the end of the null period

	   myReader. getSamples (ofdmBuffer. data (),
	                      T_u, coarseOffset + fineOffset);
	   startIndex = myCorrelator.
	                      findIndex (ofdmBuffer, 4 * THRESHOLD);
	   if (startIndex < 0) { // no sync, try again
	      isSynced	= false;
	      if (++index_attempts > 5) {
//	         syncsignalHandler (false, userData);
	         index_attempts	= 0;
	      }
	      goto notSynced;
	   }

SyncOnPhase:
	   index_attempts	= 0;
	   isSynced		= true;
	   syncsignalHandler (isSynced, userData);

//	Once here, we are synchronized, we need to copy the data we
//	used for synchronization for block 0

	   memmove (ofdmBuffer. data (),
	            &((ofdmBuffer. data ()) [startIndex]),
	                  (T_u - startIndex) * sizeof (std::complex<float>));
	   int ofdmBufferIndex	= T_u - startIndex;

//	Block 0 is special in that it is used for coarse time synchronization
//	and its content is used as a reference for decoding the
//	first datablock.
//	We read the missing samples in the ofdm buffer
	   myReader. getSamples (&((ofdmBuffer. data ()) [ofdmBufferIndex]),
	                  T_u - ofdmBufferIndex,
	                  coarseOffset + fineOffset);
	   my_ofdmDecoder. processBlock_0 (ofdmBuffer);
//
//	if correction is needed (known by the fic handler)
//	we compute the coarse offset in the phaseSynchronizer
	   correctionNeeded = !my_ficHandler. syncReached ();
	   if (correctionNeeded) {
	      int correction  = myFreqSyncer.
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
	   std::vector<int16_t> ibits (2 * params. get_carriers ());
	   for (int ofdmSymbolCount = 1;
	        ofdmSymbolCount < (uint16_t)nrBlocks; ofdmSymbolCount ++) {	
	      myReader. getSamples (ofdmBuffer. data (),
	                               T_s, coarseOffset + fineOffset);
	      for (int i = (int)T_u; i < (int)T_s; i ++) 
	         FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
//
//	Note that only the first few blocks are handled, we
//	are not interested in the "payload"
	      my_ofdmDecoder. decode (ofdmBuffer,
	                                 ofdmSymbolCount, ibits, snr);
	      if (ofdmSymbolCount < 4) 
	         my_ficHandler. processFICBlock (ibits, ofdmSymbolCount);
	   }

	   fineOffset += 0.1 * arg (FreqCorr) / M_PI * (carrierDiff);

//	at the end of the frame, just skip Tnull samples
	   myReader. getSamples (ofdmBuffer. data (),
	                         T_null, coarseOffset + fineOffset);
	   float sum	= 0;
	   for (int i = 0; i < T_null; i ++)
	      sum += abs (ofdmBuffer [i]);
	   sum /= T_null;

	   float sum2 = myReader. get_sLevel ();
	   snr	= 0.9 * snr + 0.1 * 20 * log10 ((sum2 + 0.005) / sum);
/*
 *      The TII data is encoded in the null period of the
 *      odd frames
 */
	   if (my_ficHandler. syncReached () &&
	                  my_ficHandler. evenFrame ()) {
	         my_TII_Detector. addBuffer (ofdmBuffer);
	         if (++tii_counter >= 4) {
	            std::vector<tiiData> res =
	                  my_TII_Detector. processNULL (threshold);
	            if ((res. size () > 0) && (show_tii != nullptr)) {
	               uint8_t the_ecc	= my_ficHandler. get_ecc ();
	               uint16_t the_EId	= my_ficHandler. get_EId ();
	               for (auto &d : res) {
	                  d. ecc	= the_ecc;
	                  d. EId	= the_EId;
	                  show_tii (&d, userData);
	               }
	            }
	            tii_counter = 0;
	            my_TII_Detector. reset ();
	         }
	      }

	   if (fineOffset > carrierDiff / 2) {
	      coarseOffset += carrierDiff;
	      fineOffset -= carrierDiff;
	   }
	   else
	   if (fineOffset < - carrierDiff / 2) {
	      coarseOffset -= carrierDiff;
	      fineOffset += carrierDiff;
	   }
	   goto Check_endofNull;
	}

	catch (int e) {
//	   fprintf (stderr, "dab processor will stop\n");
	}
//	fprintf (stderr, "dabProcessor is shutting down\n");
}

void	dabProcessor::reset		() {
	stop  ();
	start ();
}

void	dabProcessor::stop		() {	
	if (running. load ()) {
	   running. store (false);
	   myReader. setRunning (false);
	   sleep (1);
	   threadHandle. join ();
	}
}

void    dabProcessor::clearEnsemble	() {
	my_ficHandler. reset ();
}

std::string dabProcessor::get_ensembleName	() {
	return my_ficHandler. get_ensembleName ();
}

bool	dabProcessor::syncReached	() {
	return my_ficHandler. syncReached ();
}

int	dabProcessor::get_nrComponents	() {
	return my_ficHandler. get_nrComponents ();
}

contentType dabProcessor::content	(int compnr) {
	return my_ficHandler. content (compnr);
}

