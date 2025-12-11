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

#include	<stdint.h>
#include	<vector>
#include	<atomic>
#include	"dab-constants.h"
#include	"ringbuffer.h"
#include	"phasetable.h"
#include	"freq-interleaver.h"
#include	"fft-handler.h"

class	dabParams;

class	ofdmDecoder {
public:
		ofdmDecoder		();
		~ofdmDecoder		();
	void    processBlock_0          (std::vector<Complex>);
        void    decode                  (std::vector<Complex> &,
                                         int32_t n,
                                         std::vector<int16_t> &, float);
	void	reset			();
private:
	dabParams	params;
	interLeaver	myMapper;
	fftHandler	my_fftHandler;
	std::vector <Complex>	phaseReference;
	std::vector <Complex>	fft_buffer;
	std::vector <float>	meanLevelVector;
	std::vector <float>	sigmaSQ_Vector;
	int		cnt;
	int32_t		T_s;
	int32_t		T_u;
	int32_t		T_g;
	int32_t		carriers;
	int32_t		nrBlocks;
	int16_t		getMiddle	();
	int32_t		blockIndex;

	float		sqrt_2;
	float		meanValue;

	float		computeQuality (Complex *);
};

