#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the hackrf server
 *
 *    hackrf server is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    hackrf server is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with hackrf server; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __HACKRF_SERVER__
#define __HACKRF_SERVER__

#include	<thread>
#include	<stdint.h>
#include	<unistd.h>
#include	<atomic>
#include	"ringbuffer.h"
#include	"lowpass.h"
class	hackrfHandler;
/*
 */

class hackrf_server {
public:
		hackrf_server (int,		// frequency
	                       int,		// inputRate	 
	                       int,		// gain
	                       int,		// transmissionrate
	                       RingBuffer<uint8_t> *);
		~hackrf_server	(void);
	void		start		(void);
	void		stop		(void);
private:
	void		run		(void);
	int		inputRate;
	int		vfoFrequency;
	int		gain;
	int32_t         transmissionRate;
        int32_t         upFactor;
        LowPassFIR      *upFilter;
	hackrfHandler	*theTransmitter;
	bool		connected;
	RingBuffer<uint8_t>	*inputBuffer;
	std::thread	threadHandle;
	std::atomic<bool>	running;
};

#endif


