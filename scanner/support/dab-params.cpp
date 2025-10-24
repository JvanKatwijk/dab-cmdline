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

#include	"dab-params.h"

		dabParams::dabParams () {
	L	= 76;
	K	= 1536;
	T_F	= 196608;
	T_null	= 2656;
	T_s	= 2552;
	T_u	= 2048;
	T_g	= 504;
	carrierDiff	= 1000;
}

	dabParams::~dabParams () {
}

int16_t dabParams::get_L () {
	return L;
}

int16_t	dabParams::get_carriers () {
	return K;
}


int16_t	dabParams::get_T_null () {
	return T_null;
}

int16_t	dabParams::get_T_s () {
	return T_s;
}

int16_t	dabParams::get_T_u () {
	return T_u;
}

int16_t	dabParams::get_T_g () {
	return T_g;
}

int32_t	dabParams::get_T_F () {
	return T_F;
}

int32_t	dabParams::get_carrierDiff () {
	return carrierDiff;
}

