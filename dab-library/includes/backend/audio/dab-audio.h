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
#ifndef	__DAB_AUDIO__
#define	__DAB_AUDIO__

#include	"dab-virtual.h"
#include	"ringbuffer.h"
#include	<stdio.h>
#include	<thread>
#include	<mutex>
#include	<condition_variable>
#include	<atomic>
#include	"dab-api.h"

class	dabProcessor;
class	protection;
class	audioSink;

class	dabAudio:public dabVirtual {
public:
	dabAudio	(uint8_t dabModus,
	                 int16_t fragmentSize,
	                 int16_t bitRate,
	                 bool	shortForm,
	                 int16_t protLevel,
	                 cb_audio_t,
	                 cb_data_t);
	~dabAudio	(void);
int32_t	process		(int16_t *, int16_t);
void	stopRunning	(void);
void	start		(void);
protected:
	RingBuffer<int16_t>	*audioBuffer;
private:
void	run		(void);
	std::atomic<bool>	running;
	std::thread	threadHandle;
	uint8_t		dabModus;
	int16_t		fragmentSize;
	int16_t		bitRate;
	bool		shortForm;
	int16_t		protLevel;
	uint8_t		*outV;
	int16_t		**interleaveData;

	std::condition_variable Locker;
	std::mutex	ourMutex;

	protection	*protectionHandler;
	dabProcessor	*our_dabProcessor;
	RingBuffer<int16_t>	*Buffer;
};

#endif

