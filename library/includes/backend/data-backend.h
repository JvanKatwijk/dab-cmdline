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
#pragma once

#include	<stdio.h>
#include        <thread>
#include        <mutex>
#include	<atomic>
#include	<vector>
#include	"dab-semaphore.h"
#include	"dab-api.h"
#include	"virtual-backend.h"
#include	"ringbuffer.h"

class	backendBase;
class	protection;

class	dataBackend: public virtualBackend {
public:
		dataBackend	(packetdata *, API_struct *, void *);
		~dataBackend	();
	int32_t	process		(int16_t *, int16_t);
	void	stopRunning	();
	void	start		();
private:
	int16_t		fragmentSize;
	int16_t		bitRate;
	bool		shortForm;
	int16_t		protLevel;
void	run		(void);
	std::atomic<bool>	running;
	std::thread	threadHandle;
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
	backendBase	*our_backendBase;
};

