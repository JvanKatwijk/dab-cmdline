#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the DAB-library
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
 *
 */
#
#ifndef	__PHASEREFERENCE__
#define	__PHASEREFERENCE__

#include	"fft.h"
#include	<stdio.h>
#include	<stdint.h>
#include	"phasetable.h"
#include	"dab-constants.h"

class	dabParams;

class phaseReference : public phaseTable {
public:
		phaseReference (dabParams *, int16_t);
		~phaseReference	(void);
	int32_t	findIndex	(std::complex<float> *);
	std::complex<float>	*refTable;
private:
	int32_t		T_u;
	int16_t		threshold;

	common_fft	*fft_processor;
	std::complex<float>	*fft_buffer;
	common_ifft	*res_processor;
	std::complex<float>	*res_buffer;
	int32_t		fft_counter;
	float	Max;
};
#endif

