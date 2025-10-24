#
/*
 *    Copyright (C) 2025
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the 2-nd DAB scannner and borrowed
 *    for the Qt-DAB repository of the same author
 *
 *    DAB scannner is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB scanner is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB scanner; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#pragma once

#include	<cstdio>
#include	<cstdint>
#include	<vector>
#include	"fft-handler.h"
#include	"phasetable.h"
#include	"dab-constants.h"
#include	"dab-params.h"
#include	"ringbuffer.h"

class	RadioInterface;

class correlator {
public:
			correlator 		(phaseTable *);
			~correlator		();
	int32_t		findIndex		(std::vector<Complex>, int);
private:
	phaseTable	*theTable;
	dabParams	params;
	fftHandler	fft_forward;
	fftHandler	fft_backwards;

	std::vector<Complex> refTable;
	int16_t		depth;
	int32_t		T_u;
	int32_t		T_g;
	int16_t		carriers;

	int32_t		fft_counter;
	int32_t		framesperSecond;	
	int32_t		displayCounter;
};

