#
/*
 *    Copyright (C) 2020 .. 2025
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

#pragma once

#include	<atomic>
#include	<iio.h>
#include	"ringbuffer.h"
#include	"device-handler.h"
#include	<thread>

#define	PLUTO_RATE	2100000
#define	DAB_RATE	2048000
#define	DIVIDER		1000
#define	CONV_SIZE	(PLUTO_RATE / DIVIDER)
class	plutoHandler: public deviceHandler {
public:
			plutoHandler		(int	frequency,
	                                         int	gain,
	                                         bool	agc);
	    		~plutoHandler		();
	int32_t		defaultFrequency	();
	bool		restartReader		(int32_t);
	void		stopReader		();
	int32_t		getSamples		(std::complex<float> *,
	                                                          int32_t);
	int32_t		Samples			();
	void		resetBuffer		();
	int16_t		bitDepth		();

private:

	RingBuffer<std::complex<float>> _I_Buffer;
	std::thread		threadHandle;
	void			run		();
	int32_t			inputRate;
	std::atomic<bool>	running;
//	configuration items
	int64_t			bw_hz; // Analog banwidth in Hz
	int64_t			fs_hz; // Baseband sample rate in Hz
	int64_t			lo_hz; // Local oscillator frequency in Hz
	const char* rfport; // Port name
	struct	iio_channel	*lo_channel;
	struct	iio_channel	*gain_channel;
	struct	iio_device	*rx;
	struct	iio_context	*ctx;
	struct	iio_channel	*rx0_i;
	struct	iio_channel	*rx0_q;
	struct	iio_buffer	*rxbuf;
//	struct	stream_cfg	rxcfg;
	std::complex<float>	convBuffer	[CONV_SIZE + 1];
	int			convIndex;
	int16_t			mapTable_int	[DAB_RATE / DIVIDER];
	float			mapTable_float	[DAB_RATE / DIVIDER];
};



