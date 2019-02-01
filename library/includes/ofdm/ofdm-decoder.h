#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the  DAB-library
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
#ifndef	__OFDM_DECODER__
#define	__OFDM_DECODER__

#include	<stdint.h>
#include	<vector>
#include	<atomic>
#include	"dab-constants.h"
#include	"ringbuffer.h"
#include	"phasetable.h"
#include	"freq-interleaver.h"
#include	"fft_handler.h"

class	dabParams;

class	ofdmDecoder {
public:
		ofdmDecoder		(uint8_t	dabMode,
                                         RingBuffer<std::complex<float>> *);
		~ofdmDecoder		(void);
	void	processBlock_0		(std::complex<float> *);
	void	decode		(std::complex<float> *, int32_t n, int16_t *);
private:
	dabParams	params;
	fft_handler	my_fftHandler;
	interLeaver	myMapper;
        RingBuffer<std::complex<float>> *iqBuffer;
	int		cnt;
	int32_t		T_s;
	int32_t		T_u;
	int32_t		T_g;
	int32_t		carriers;
	int32_t		nrBlocks;
	int16_t		getMiddle	(void);
	std::vector <complex<float> >	phaseReference;
	std::complex<float>	*fft_buffer;
	int32_t		blockIndex;
};

#endif


