#
/*
 *    Copyright (C) 2013 .. 2017
 *	update 2023
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

		mscHandler::mscHandler	(API_struct	*p,
	                                 void		*userData):
	                                    params (p -> dabMode) {
	this	-> p			= p;
	this	-> soundOut		= p -> audioOut_Handler;
	this	-> dataOut		= p -> dataOut_Handler;
	this	-> bytesOut		= p -> bytesOut_Handler;
	this	-> programQuality	= p -> program_quality_Handler;
	this	-> motdata_Handler	= p -> motdata_Handler;
	this	-> userData		= userData;
//	cifVector. resize (55296);
	cifCount		= 0;	// msc blocks in CIF
	theBackends. push_back (new virtualBackend (0, 0));
	BitsperBlock		= 2 * params. get_carriers ();
	numberofblocksperCIF	= blocksperCIF [(p -> dabMode - 1) & 03];

	work_to_do. store (false);
	running. store (false);
	phaseReference. resize (params. get_T_u ());
}

	mscHandler::~mscHandler	() {
}

void	mscHandler::stop () {
	locker. lock ();
	for (auto const &b : theBackends) {
	   b -> stopRunning ();
	   delete b;
	}

	theBackends. resize (0);
	work_to_do. store (false);
	locker. unlock ();
}

void	mscHandler::start	(void) {
	if (running.load ()) {
	   fprintf (stderr, "cannot restart mscHandler, still active\n");
	   return;
	}
}

void	mscHandler::reset	() {
	stop ();
	start ();
}
//
//	Note, the set_xxx functions are called from within a
//	different thread than the process_mscBlock method,
//	so, a little bit of locking seems wise while
//	the actual changing of the settings is done in the
//	thread executing process_mscBlock
void	mscHandler::set_audioChannel (audiodata &d) {
	locker. lock ();
//
//	we could assert here that theBackend == nullptr
	theBackends. push_back (new audioBackend (&d, p, userData));
	work_to_do. store (true);
	locker. unlock ();
}

void	mscHandler::set_dataChannel (packetdata &d) {
	locker. lock ();
	theBackends. push_back (new dataBackend (&d, p, userData));
	work_to_do. store (true);
	locker. unlock ();
}

void	mscHandler::process_mscBlock	(std::vector<int16_t> &fbits,
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
	locker. lock ();
	cifCount	= (cifCount + 1) & 03;
	for (auto const& b: theBackends) {
	   int startAddr	= b -> startAddr ();
	   int Length		= b -> Length    ();

	   if (Length > 0) {
#ifdef _MSC_VER
	      int16_t *myBegin = (int16_t *)_alloca((Length * CUSize) * sizeof(int16_t));
#else
	      int16_t myBegin [Length * CUSize];
#endif
	      memcpy (myBegin, &cifVector [startAddr * CUSize],
	                               Length * CUSize * sizeof (int16_t));
	      (void) b -> process (myBegin, Length * CUSize);
	   }
	}
	locker. unlock ();
}

