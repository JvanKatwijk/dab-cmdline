#
/*
 *    Copyright (C) 2017 .. 2018
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB cmdline
 *
 *    DAB cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB cmdline; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __HACKRF_HANDLER__
#define	__HACKRF_HANDLER__

#include	"ringbuffer.h"
#include	<atomic>
#include	"device-handler.h"
#include	"libhackrf/hackrf.h"

typedef int (*hackrf_sample_block_cb_fn)(hackrf_transfer *transfer);


///////////////////////////////////////////////////////////////////////////
class	hackrfHandler: public deviceHandler {
public:
			hackrfHandler		(int32_t frequency,
	                                         int16_t  ppm,
                                                 int16_t  lnaGain,
                                                 int16_t  vgaGain,
	                                         bool	ampEnable = false);
			~hackrfHandler		(void);
	bool		restartReader		(int32_t);
	void		stopReader		(void);
	int32_t		getSamples		(std::complex<float> *,
	                                                          int32_t);
	int32_t		Samples			(void);
	void		resetBuffer		(void);
	int16_t		bitDepth		(void);
//
//	The buffer should be visible by the callback function
	RingBuffer<std::complex<float>>	*_I_Buffer;
	hackrf_device	*theDevice;
private:
	int32_t		vfoFrequency;
	int16_t		lnaGain;
	int16_t		vgaGain;
	bool		ampEnable;
	int32_t		inputRate;
	std::atomic<bool>	running;
	void		setLNAGain		(int);
	void		setVGAGain		(int);
};
#endif

