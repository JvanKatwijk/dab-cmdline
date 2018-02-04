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
//
		mscHandler::mscHandler	(uint8_t	Mode,
	                                 audioOut_t	soundOut,
	                                 dataOut_t	dataOut,
	                                 bytesOut_t	bytesOut,
	                                 programQuality_t mscQuality,
	                                 motdata_t	motdata_Handler,
	                                 void		*userData):
	                                    params (Mode) {
	this	-> soundOut		= soundOut;
	this	-> dataOut		= dataOut;
	this	-> bytesOut		= bytesOut;
	this	-> programQuality	= mscQuality;
	this	-> motdata_Handler	= motdata_Handler;
	this	-> userData		= userData;

	cifVector. resize (55296);
	cifCount		= 0;	// msc blocks in CIF
	blkCount		= 0;
	theBackend		= new virtualBackend (0, 0);
	BitsperBlock		= 2 * params. get_carriers ();
	if (Mode == 4)	// 2 CIFS per 76 blocks
	   numberofblocksperCIF	= 36;
	else
	if (Mode == 1)	// 4 CIFS per 76 blocks
	   numberofblocksperCIF	= 18;
	else
	if (Mode == 2)	// 1 CIF per 76 blocks
	   numberofblocksperCIF	= 72;
	else			// shouldnot/cannot happen
	   numberofblocksperCIF	= 18;
}

		mscHandler::~mscHandler	(void) {
	delete theBackend;
}
//
//	Note, the set_xxx functions are called from within a
//	different thread than the process_mscBlock method,
//	so, a little bit of locking seems wise while
//	the actual changing of the settings is done in the
//	thread executing process_mscBlock
void	mscHandler::set_audioChannel (audiodata *d) {
	mutexer. lock ();
	delete theBackend;
	theBackend 	= new audioBackend (d,
	                                    soundOut,
	                                    dataOut,
	                                    programQuality,
	                                    motdata_Handler,
	                                    userData);
	mutexer. unlock ();
}


void	mscHandler::set_dataChannel (packetdata *d) {
	mutexer. lock ();
	delete theBackend;
	theBackend	= new dataBackend (d,
	                                   bytesOut,
	                                   userData);
	mutexer. unlock ();
}

void	mscHandler::process_mscBlock	(std::vector<int16_t> fbits,
	                                 int16_t blkno) { 
int16_t	currentblk;
std::vector<int16_t> myBegin;
int16_t	startAddr;
int16_t	Length;
//
//	we accept the incoming data
	currentblk	= (blkno - 4) % numberofblocksperCIF;
	memcpy (&cifVector [currentblk * BitsperBlock],
	                    fbits. data (), BitsperBlock * sizeof (int16_t));
	if (currentblk < numberofblocksperCIF - 1) 
	   return;

//	OK, now we have a full CIF
	mutexer. lock ();
//	these we need for actual processing
	startAddr	= theBackend -> startAddr ();
	Length		= theBackend -> Length    ();
//
	blkCount	= 0;
	cifCount	= (cifCount + 1) & 03;
	myBegin. resize (Length * CUSize);
	memcpy (myBegin. data (), &cifVector [startAddr * CUSize],
	                               Length * CUSize * sizeof (int16_t));
	mutexer. unlock ();
	(void) theBackend -> process (myBegin. data (), Length * CUSize);
}
//

void	mscHandler::stopProcessing (void) {
	work_to_be_done	= false;
}

void	mscHandler::stopHandler	(void) {
	work_to_be_done	= false;
	theBackend	-> stopRunning ();
}

