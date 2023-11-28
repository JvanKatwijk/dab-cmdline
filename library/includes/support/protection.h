#
/*
 *    Copyright (C) 2017
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
 *
 *	Simple base class for combining uep and eep deconvolvers
 */
#pragma once

#include	<stdint.h>
#include	<vector>
#include	"viterbi-spiral.h"
//#include	"viterbi-handler.h"

class	protection: public viterbiSpiral {
//class	protection: public viterbiHandler {
public:
		protection  	(int16_t, int16_t);
virtual		~protection	();
virtual	bool	deconvolve	(int16_t *, int32_t, uint8_t *);
protected:
        int16_t         bitRate;
        int32_t         outSize;
        std::vector<uint8_t> indexTable;
        std::vector<int16_t> viterbiBlock;
};

