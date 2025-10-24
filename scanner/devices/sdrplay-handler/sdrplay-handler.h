#
/*
 *    Copyright (C) 2016 .. 2025
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming
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

#include	<dlfcn.h>
#include	<atomic>
#include	"ringbuffer.h"
#include	"device-handler.h"
#include	"mirsdrapi-rsp.h"

typedef void (*mir_sdr_StreamCallback_t)(int16_t	*xi,
	                                 int16_t	*xq,
	                                 uint32_t	firstSampleNum, 
	                                 int32_t	grChanged,
	                                 int32_t	rfChanged,
	                                 int32_t	fsChanged,
	                                 uint32_t	numSamples,
	                                 uint32_t	reset,
	                                 uint32_t	hwRemoved,
	                                 void		*cbContext);
typedef	void	(*mir_sdr_GainChangeCallback_t)(uint32_t	gRdB,
	                                        uint32_t	lnaGRdB,
	                                        void		*cbContext);

///////////////////////////////////////////////////////////////////////////
class	sdrplayHandler: public deviceHandler {
public:
		sdrplayHandler          (int32_t        frequency,
	                                 int16_t        ppmCorrection,
	                                 int16_t	GRdB,
	                                 int16_t	lnaState,
	                                 bool		autogain,
	                                 uint16_t       deviceIndex,
	                                 int16_t        antenna,
	                                 bool		X_dump = false);

		~sdrplayHandler		();

	bool	restartReader		(int32_t);
	void	stopReader		();
	int32_t	getSamples		(std::complex<float> *, int32_t);
	int32_t	Samples			();
	void	resetBuffer		();
	int16_t	bitDepth		();
//
//	need to be visible, since being accessed from 
//	within the callback
	RingBuffer<std::complex<float>>	*_I_Buffer;
	float		denominator;
private:

	int16_t		hwVersion;
	int16_t		nrBits;
	uint16_t	deviceIndex;
	uint32_t	numofDevs;	// int32_t not my choice
	int32_t		inputRate;
	int32_t		frequency;
	int16_t		ppmCorrection;
	int16_t		GRdB;
	int16_t		lnaState;
	std::atomic<bool>	running;
	mir_sdr_AgcControlT agcMode;
	bool		X_dump;
	void		setup_xmlDump	();
	void		close_xmlDump	();
};


