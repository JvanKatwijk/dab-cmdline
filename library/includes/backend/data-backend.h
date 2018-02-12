#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library of the SDR-J (JSDR).
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#
#ifndef	__DATA_BACKEND__
#define	__DATA_BACKEND__

#include	<stdio.h>
#include        <thread>
#include        <mutex>
#include	<atomic>
#include	<vector>
#include	"semaphore.h"
#include	"dab-api.h"
#include	"virtual-backend.h"
#include	"ringbuffer.h"

class	dabProcessor;
class	protection;

class	dataBackend: public virtualBackend {
public:
		dataBackend	(packetdata	*,
	                         bytesOut_t	bytesOut,
	                         motdata_t	motdataHandler,
	                         void		*userData);
		~dataBackend	(void);
	int32_t	process		(int16_t *, int16_t);
	void	stopRunning	(void);
	void	start		(void);
private:
	uint8_t		DSCTy;
	int16_t		fragmentSize;
	int16_t		bitRate;
	bool		shortForm;
	int16_t		protLevel;
	uint8_t		DGflag;
	int16_t		FEC_scheme;
	bool		show_crcErrors;
	int16_t		crcErrors;
void	run		(void);
	std::atomic<bool>	running;
	std::thread	threadHandle;
	int16_t		interleaverIndex;
	int16_t		countforInterleaver;
	std::vector<uint8_t> outV;
	std::vector<int16_t>	tempX;
	std::vector<uint8_t>	disperseVector;
	int16_t		**interleaveData;
	Semaphore	freeSlots;
	Semaphore	usedSlots;

	int16_t		*theData [20];
	int16_t		nextIn;
	int16_t		nextOut;

	protection	*protectionHandler;
	dabProcessor	*our_dabProcessor;
};

#endif

