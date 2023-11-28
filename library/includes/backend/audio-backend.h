#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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
#pragma once

#include	<stdio.h>
#include	<thread>
#include	<mutex>
#include	<condition_variable>
#include	<atomic>
#include	<vector>
#include	"dab-api.h"
#include	"virtual-backend.h"
#include	"ringbuffer.h"
#include	"dab-semaphore.h"

class	backendBase;
class	protection;
class	audioSink;

class	audioBackend:public virtualBackend {
public:
	audioBackend	(audiodata *, API_struct *, void	*);
	~audioBackend	(void);
int32_t	process		(int16_t *, int16_t);
void	stopRunning	(void);
void	start		(void);
private:
	void		run		(void);
	void		processSegment	(int16_t *);

	std::atomic<bool>	running;
	std::thread	threadHandle;
	uint8_t		dabModus;
	int16_t		fragmentSize;
	int16_t		bitRate;
	bool		shortForm;
	int16_t		protLevel;
	std::vector<uint8_t> outV;
	std::vector<uint8_t> disperseVector;
	int16_t		**interleaveData;
	int16_t		interleaverIndex;
	int16_t		countforInterleaver;
	std::vector<int16_t> tempX;

	Semaphore	freeSlots;
	Semaphore	usedSlots;
	int16_t		nextIn;
	int16_t		nextOut;
	int16_t		*theData [20];

	protection	*protectionHandler;
	backendBase	*our_backendBase;
};

