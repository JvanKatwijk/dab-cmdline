#
/*
 *    Copyright (C) 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the simpleDab program
 *    simpleDab is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    simpleDab is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with simpleDab; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
//
//	Common definitions and includes for
//	the DAB decoder

#ifndef	__SIMPLEDAB_CONSTANTS__
#define	__SIMPLEDAB_CONSTANTS__
#
#include	<math.h>
#include	<stdint.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<complex>
#include	<limits>
#include	<cstring>
#include	<unistd.h>

#ifdef	__MINGW32__
#include	"windows.h"
#else
#ifndef	__FREEBSD__
#include	"alloca.h"
#endif
#include	"dlfcn.h"
typedef	void	*HINSTANCE;
#endif

typedef	float	DSPFLOAT;
typedef	std::complex<DSPFLOAT> DSPCOMPLEX;

using namespace std;
#endif
