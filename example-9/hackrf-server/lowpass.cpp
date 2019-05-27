#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the hackrf transmitter
 *
 *    hackrf transmitter is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    hackrf transmitter is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with hackrf transmitter; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"lowpass.h"

//FIR LowPass

	LowPassFIR::LowPassFIR (int16_t firsize,
	                        int32_t Fc,
	                        int32_t fs) {
int16_t	i;
float	tmp [firsize];
float	f	= (float)Fc / fs;
float	sum	= 0.0;

	this	-> firsize	= firsize;
	buffer. resize (firsize);
	kernel. resize (firsize);
	for (i = 0; i < firsize; i ++) {
	   if (i == firsize / 2)
	      tmp [i] = 2 * M_PI * f;
	   else 
	      tmp [i] = sin (2 * M_PI * f * (i - firsize/2))/ (i - firsize/2);
//
//	Blackman window
	   tmp [i]  *= (0.42 -
		    0.5 * cos (2 * M_PI * (float)i / firsize) +
		    0.08 * cos (4 * M_PI * (float)i / firsize));

	   sum += tmp [i];
	}

	for (i = 0; i < firsize; i ++)
	   kernel [i] = std::complex<float> (tmp [i] / sum, 0);
	ip	= 0;
}

	LowPassFIR::~LowPassFIR () {
}

//
//	we process the samples backwards rather than reversing
//	the kernel
std::complex<float>	LowPassFIR::Pass (std::complex<float> z) {
int16_t	i;
std::complex<float>	tmp	= std::complex<float> (0, 0);

	buffer [ip]	= z;
	for (i = 0; i < firsize; i ++) {
	   int16_t index = ip - i;
	   if (index < 0)
	      index += firsize;
	   tmp		+= buffer [index] * kernel [i];
	}

	ip = (ip + 1) % firsize;
	return tmp;
}
