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

class	dabParams {
public:
			dabParams 	();
			~dabParams 	();
	int16_t		get_L 		();
	int16_t		get_carriers 	();
	int16_t		get_T_null	();
	int16_t		get_T_s		();
	int16_t		get_T_u 	();
	int16_t		get_T_g 	();
	int32_t		get_T_F 	();
	int32_t		get_carrierDiff ();
private:
	int16_t	L;
	int16_t	K;
	int16_t	T_null;
	int32_t	T_F;
	int16_t	T_s;
	int16_t	T_u;
	int16_t	T_g;
	int16_t	carrierDiff;
};

