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
 */
#
#pragma once

#include	<stdio.h>
#include	<stdint.h>
#include	<vector>
#include	"dab-constants.h"
#include	"dab-params.h"

struct phasetableElement {
	int32_t	kmin, kmax;
	int32_t i;
	int32_t n;
};


class phaseTable {
public:
		phaseTable 	(int16_t);
		~phaseTable	();
	std::vector<std::complex<float>>        refTable;
	float	get_Phi		(int32_t);
private:
	dabParams	params;
	int16_t		T_u;
	struct phasetableElement	*currentTable;
	int16_t		Mode;
	int32_t		h_table (int32_t i, int32_t j);
};

