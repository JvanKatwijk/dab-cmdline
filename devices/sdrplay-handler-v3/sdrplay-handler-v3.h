#
/*
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of dab cmdline programs
 *
 *    Dab cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Dab cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab cmdline; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SDRPLAY_HANDLER_V3__
#define	__SDRPLAY_HANDLER_V3__

#include	<atomic>
#include	<stdio.h>
#include	"dab-constants.h"
#include	"ringbuffer.h"
#include	"device-handler.h"

class	sdrplayController;

class	sdrplayHandler_v3: public deviceHandler {
public:
		sdrplayHandler_v3 (uint32_t        frequency,
                                   int16_t        ppmCorrection,
                                   int16_t        GRdB,
                                   int16_t        lnaState,
                                   bool           autogain,
                                   uint16_t       deviceIndex,
                                   int16_t        antenna);

		~sdrplayHandler_v3	();
	bool    restartReader           (int32_t);
        void    stopReader              (void);
        int32_t getSamples              (std::complex<float> *, int32_t);
        int32_t Samples                 (void);
        void    resetBuffer             (void);
        int16_t bitDepth                (void);
	float	denominator;
private:
	RingBuffer<std::complex<float>>	*_I_Buffer;
	sdrplayController	*theController;
	int32_t			vfoFrequency;
	int16_t			ppmCorrection;
	int16_t			GRdB;
	int16_t			lnaState;
	bool			autogain;
	uint16_t		deviceIndex;
	int16_t			antenna;
	int16_t			nrBits;
};
#endif

