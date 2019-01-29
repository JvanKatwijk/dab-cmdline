#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB program
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__TII_DETECTOR__
#define	__TII_DETECTOR__

#include	<stdint.h>
#include	"dab-params.h"
#include	"fft_handler.h"
#include	<vector>

class	TII_Detector {
public:
		TII_Detector	(uint8_t dabMode);
		~TII_Detector	(void);
	void	reset		(void);
	void	addBuffer	(std::vector<std::complex<float>>,
	                         float alfa = -1.0F,
	                         int32_t cifCounter =-1);
inline	uint16_t getNumBuffers	(void) const { return numUsedBuffers; }
	void	processNULL	(int16_t *, int16_t *);
	void	processNULL_ex	(int *pNumOut, int *outTii,
	                         float *outAvgSNR,
	                         float *outMinSNR,
	                         float *outNxtSNR);

private:
	void	collapse  (std::complex<float> *, float *);
	uint8_t	invTable [256];
	dabParams	params;
	fft_handler	my_fftHandler;
	uint16_t	numUsedBuffers;
	int16_t		T_u;
	int16_t		carriers;
	bool		ind;
	std::complex<float>	*fft_buffer;
	std::vector<complex<float> >	theBuffer;
	std::vector<float>	window;
public:
	float		P_allAvg  [2048];
private:
	float		P_tmpNorm [2048];
	float		P_avg     [384];	// 8 groups per 24*2 carriers = 384 carriers

	bool		isFirstAdd;
	int16_t		fillCount;
};

#endif

