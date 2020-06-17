#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library
 *
 *    DAB library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__STREAMER__
#define	__STREAMER__
#include        "ringbuffer.h"
#include        <thread>
#include        <atomic>
#include	<stdint.h>
#include	<stdio.h>

class	streamer {
public:
	streamer		(void);	
	~streamer		(void);
void	addBuffer		(void *, int, int);
bool	restart			(void);
bool	isRunning		(void);
void	stop			(void);
private:
void	run			(void);
	std::atomic<bool>	running;
	RingBuffer<int16_t> *theBuffer;
	std::thread	workerHandle;
};

#endif
