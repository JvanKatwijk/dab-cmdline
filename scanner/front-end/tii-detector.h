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

#include	<cstdint>
#include	"dab-params.h"
#include	"fft-handler.h"
#include	"phasetable.h"
#include	<vector>

#define NUM_GROUPS      8
#define GROUPSIZE       24


typedef std::complex<float> Complex;
typedef struct {
        int index;
        float value;
        bool    norm;
} resultPair;


class	TII_Detector {
public:
			TII_Detector	();
			~TII_Detector	();
	void		reset		();
	void		addBuffer	(const std::vector<Complex> &);
	std::vector<tiiData> processNULL	(int16_t);


private:
	dabParams	params;
	phaseTable	theTable;
	std::vector<Complex> table_2;
	void		resetBuffer	();
	uint16_t	getPattern	(int);
	uint16_t	nrPatterns	();
	std::vector<Complex>	nullSymbolBuffer;
	std::vector<float>	window;
	int16_t		T_u;
	int16_t		T_g;
	int16_t		carriers;	
	fftHandler	my_fftHandler;
	Complex		decodedBuffer [768];
	void		collapse	(const Complex *, 
	                                 Complex *, Complex *, bool);

	int		tiiThreshold;
};


