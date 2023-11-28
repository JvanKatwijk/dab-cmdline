#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB-library
 *
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
#include	<stdint.h>
#include	<vector>
#include	"phasetable.h"
#include	"dab-constants.h"
#include	"fft-handler.h"
#include	"dab-params.h"

class phaseReference : public phaseTable {
public:
		phaseReference (uint8_t, int16_t);
		~phaseReference	();
	int32_t	findIndex	(std::complex<float> *, int);
	int16_t	estimateOffset	(std::complex<float> *);
private:
	std::vector<std::complex<float>>        refTable;
	std::vector<float>      phaseDifferences;
	dabParams		params;
	int32_t			T_u;
	int32_t			T_g;
	int16_t			diff_length;
	int16_t			shiftFactor;
	fft_handler	my_fftHandler;
	std::complex<float>     *fft_buffer;
};

