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
#include	"dab-processor.h"
#include	"device-handler.h"
#include	"timesyncer.h"
#include	"dab-api.h"

/**
  *	\brief dabProcessor
  *	The dabProcessor class is the driver of the processing
  *	of the samplestream.
  */

	dabProcessor::dabProcessor	(deviceHandler	*inputDevice,
	                                 uint8_t	dabMode,
	                                 syncsignal_t	syncsignalHandler,
	                                 systemdata_t	systemdataHandler,
	                                 ensemblename_t	ensemblename_Handler,
	                                 programname_t	programname_Handler,
	                                 fib_quality_t	fibquality_Handler,
	                                 audioOut_t	audioOut,
	                                 bytesOut_t	bytesOut,
	                                 dataOut_t	dataOut_handler,
	                                 programdata_t	programdata,
	                                 programQuality_t mscQuality,
	                                 motdata_t	motdata_Handler,
	                                 RingBuffer<std::complex<float>> *spectrumBuffer,
	                                 RingBuffer<std::complex<float>> *iqBuffer,
	                                 void		*userData):
	                                    tii_framedelay(20),
	                                    tii_counter(0),
	                                    my_tiiHandler(nullptr),
	                                    tii_alfa(-1.0F),
	                                    tii_resetFrameCount(-1),
	                                    tii_num( 0 ),
	                                    params (dabMode),
	                                    myReader (this,
	                                              inputDevice,
	                                              spectrumBuffer),
	                                    phaseSynchronizer (dabMode,
	                                                       THRESHOLD,
	                                                       DIFF_LENGTH),
	                                    my_TII_Detector (dabMode),
	                                    my_ofdmDecoder (dabMode,
	                                                    iqBuffer),
	                                    my_ficHandler (dabMode,
	                                                   ensemblename_Handler,
                                                           programname_Handler,
                                                           fibquality_Handler,
	                                                   userData),
	                                    my_mscHandler  (dabMode,
                                                            audioOut,
                                                            dataOut_handler,
                                                            bytesOut,
                                                            mscQuality,
                                                            motdata_Handler,
                                                            userData) {
	this	-> inputDevice		= inputDevice;
	this	-> syncsignalHandler	= syncsignalHandler;
	this	-> systemdataHandler	= systemdataHandler;
	this	-> userData		= userData;
	this	-> T_null		= params. get_T_null ();
	this	-> T_s			= params. get_T_s ();
	this	-> T_u			= params. get_T_u ();
	this	-> T_F			= params. get_T_F ();
	this	-> T_g			= params. get_T_g();
	this	-> nrBlocks		= params. get_L ();
	this	-> carriers		= params. get_carriers ();
	this	-> carrierDiff		= params. get_carrierDiff ();
	isSynced	= false;
	running. store (false);
}

	dabProcessor::~dabProcessor	(void) {
	stop ();
}

void	dabProcessor::start	(void) {
	if (running. load ())
	   return;
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
void	dabProcessor::run	(void) {
std::complex<float>	FreqCorr;
timeSyncer      myTimeSyncer (&myReader);
int32_t		i;
float		fineOffset	= 0;
float		coarseOffset	= 0;
bool		correctionNeeded	= true;
std::vector<complex<float>>	ofdmBuffer (T_null);
int		dip_attempts		= 0;
int		index_attempts		= 0;

	isSynced	= false;
	running. store (true);
	my_ficHandler. reset ();
	myReader. setRunning (true);
	my_mscHandler. start ();
	try {
	   for (i = 0; i < T_F / 2; i ++) {
	      jan_abs (myReader. getSample (0));
	   }

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
//	We arrive here when - it seems we are time synchronized,
//	either from above
//	or after having processed a frame
//	Now read in Tu samples. The precise number is not really important
//	as long as we can be sure that the first sample to be identified
//	is part of the samples read.
	   myReader. getSamples (ofdmBuffer. data (),
	                         T_u, coarseOffset + fineOffset);
	   int startIndex = phaseSynchronizer. findIndex (ofdmBuffer. data ());
	   if (startIndex < 0) { // no sync, try again
	      isSynced	= false;
	      if (++index_attempts > 5) {
	         syncsignalHandler (false, userData);
	         index_attempts	= 0;
	      }
	      goto notSynced;
	   }

	   index_attempts	= 0;
	   dip_attempts		= 0;
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
	   my_ofdmDecoder. processBlock_0 (ofdmBuffer. data ());
	   my_mscHandler.  process_mscBlock (ofdmBuffer. data (), 0);
//
//	if correction is needed (known by the fic handler)
//	we compute the coarse offset in the phaseSynchronizer
	   correctionNeeded = !my_ficHandler. syncReached ();
	   if (correctionNeeded) {
	      int correction  = phaseSynchronizer.
	                                  estimateOffset (ofdmBuffer. data ());
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
	      for (i = (int)T_u; i < (int)T_s; i ++) 
	         FreqCorr += ofdmBuffer [i] * conj (ofdmBuffer [i - T_u]);
//
//	Note that only the first few blocks are handled locally
//	The FIC/FIB handling is in this thread, so that there is
//	no delay is "knowing" that we are synchronized
	      if (ofdmSymbolCount < 4) {
	         my_ofdmDecoder. decode (ofdmBuffer. data (),
	                                 ofdmSymbolCount, ibits. data ());
	         my_ficHandler. process_ficBlock (ibits, ofdmSymbolCount);
	      }
	      my_mscHandler. process_mscBlock (&((ofdmBuffer. data ()) [T_g]),
	                                                   ofdmSymbolCount);
	   }

//	we integrate the newly found frequency error with the
//	existing frequency error.
	   fineOffset += 0.1 * arg (FreqCorr) / M_PI * (carrierDiff);

//	at the end of the frame, just skip Tnull samples
	   myReader. getSamples (ofdmBuffer. data (),
	                         T_null, coarseOffset + fineOffset);
	   float sum	= 0;
	   for (i = 0; i < T_null; i ++)
	      sum += abs (ofdmBuffer [i]);
	   sum /= T_null;

	   float sum2 = myReader. get_sLevel ();
	static int ccc	= 0;
	if (++ccc > 10) {
	   ccc = 0;
//	   fprintf (stderr, "%f\n", 20 * log10 ((sum2 + 0.005) / sum));
	}

/*
 *	The TII data is encoded in the null period of the
 *	odd frames 
 */
	   if (params. get_dabMode () == 1 && my_tiiHandler) {
	      if (wasSecond (my_ficHandler. get_CIFcount (), &params)) {
	         my_TII_Detector. addBuffer (ofdmBuffer, tii_alfa );	// forward tii_algo to addBuffer()
	         ++tii_num;	// number of incremented buffer entries
	         ++tii_counter;
	         if ( tii_counter >= tii_framedelay ) {
	            int16_t mainId	= -1;
	            int16_t subId	= -1;
	            my_TII_Detector. processNULL (&mainId, &subId);		// forward tii_algo to processNULL()
	            my_tiiHandler( mainId, subId, tii_num, userData );
	         }
	         if (tii_counter >= tii_framedelay)
	            tii_counter = 0;
	         if (tii_num >= tii_resetFrameCount && tii_resetFrameCount > 0) {
	            my_TII_Detector. reset();
	            tii_num = 0;
	         }
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
	   goto SyncOnPhase;
	}
	
	catch (int e) {
	   ;
	}
	my_mscHandler.  stop ();
//	fprintf (stderr, "dabProcessor is shutting down\n");
}

void	dabProcessor:: reset	(void) {
	stop  ();
	start ();
}

void	dabProcessor::stop	(void) {	
	if (running. load ()) {
	   running. store (false);
	   myReader. setRunning (false);
	   sleep (1);
	   threadHandle. join ();
	}
}

void	dabProcessor::call_systemData (bool f, int16_t snr, int32_t freq) {
	if (systemdataHandler != nullptr)
	   systemdataHandler (f, snr, freq, userData);
}

void	dabProcessor::show_Corrector (int freqOffset) {
	if (systemdataHandler != nullptr)
	   systemdataHandler (isSynced,
	                      my_ofdmDecoder. get_snr (),
	                      freqOffset, userData);
}

bool	dabProcessor::signalSeemsGood	(void) {
	return isSynced;
}
//
//	to be handled by delegates
uint8_t dabProcessor::kindofService	(std::string s) {
        return my_ficHandler. kindofService (s);
}

void    dabProcessor::dataforAudioService	(std::string s,audiodata *dd) {
        my_ficHandler. dataforAudioService (s, dd, 0);
}

void    dabProcessor::dataforAudioService	(std::string s,
                                                  audiodata *d, int16_t c) {
        my_ficHandler. dataforAudioService (s, d, c);
}

void    dabProcessor::dataforDataService	(std::string s,
                                                    packetdata *d, int16_t c) {
        my_ficHandler. dataforDataService (s, d, c);
}

int32_t	dabProcessor::get_SId		(std::string s) {
	return my_ficHandler. SIdFor (s);
}

std::string dabProcessor::get_serviceName (int32_t SId) {
	return my_ficHandler. nameFor (SId);
}

void    dabProcessor::reset_msc (void) {
        my_mscHandler. reset ();
}

void    dabProcessor::setTII_handler(tii_t tii_Handler, int framedelay, float alfa, int resetFrameCount) {
	if ( framedelay > 0 )
		tii_framedelay = framedelay;
	my_tiiHandler = tii_Handler;
	tii_alfa = alfa;
	tii_resetFrameCount = resetFrameCount;
}

std::complex<float>
	dabProcessor::get_coordinates	(int16_t mainId, int16_t subId,
	                                 bool *success) {
        return my_ficHandler. get_coordinates (mainId, subId, success);
}

std::complex<float>
	dabProcessor::get_coordinates	(int16_t mainId, int16_t subId,
	                                 bool *success,
	                                 int16_t *pMainId, int16_t *pSubId,
	                                 int16_t *pTD) {
        return my_ficHandler. get_coordinates (mainId, subId, success,
	                                            pMainId, pSubId, pTD);
}

uint8_t dabProcessor::getECC    (bool *success) {
        return my_ficHandler. getECC (success);
}

uint8_t dabProcessor::getInterTabId     (bool *success) {
        return my_ficHandler. getInterTabId (success);
}


void    dabProcessor::set_audioChannel (audiodata *d) {
        my_mscHandler. set_audioChannel (d);
}

void    dabProcessor::set_dataChannel (packetdata *d) {
	my_mscHandler. set_dataChannel (d);
}

void    dabProcessor::clearEnsemble     (void) {
        my_ficHandler. reset ();
}

bool	dabProcessor::wasSecond (int16_t cf, dabParams *p) {
	switch (p -> get_dabMode ()) {
	   default:
	   case 1:
	      return (cf & 07) >= 4;
	   case 2:
	   case 3:
	      return (cf & 02);
	   case 4:
	      return (cf & 03) >= 2;
	}
}


