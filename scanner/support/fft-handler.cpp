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

#include	"fft-handler.h"
#include	<cstring>

#include	"fft-handler.h"

	fftHandler::fftHandler	(int size, bool dir) {
	this	-> size		= size;
	this	-> dir		= dir;

	fftVector		= (Complex *)
	                          fftwf_malloc (sizeof (Complex) * size);
	plan			= fftwf_plan_dft_1d (size,
	                           reinterpret_cast <fftwf_complex *>(fftVector),
                                   reinterpret_cast <fftwf_complex *>(fftVector),
                                   FFTW_FORWARD, FFTW_ESTIMATE);
}

	fftHandler::~fftHandler	() {
	fftwf_destroy_plan (plan);
	fftwf_free (fftVector);
}

void	fftHandler::fft		(std::vector<Complex> &v) {
	if (dir)
	   for (int i = 0; i < size; i ++)
	      fftVector [i] = conj (v [i]);
	else
	   for (int i = 0; i < size; i ++)
	      fftVector [i] = v [i];
	fftwf_execute (plan);
	if (dir)
	   for (int i = 0;  i < size; i ++)
	      v [i] = conj (fftVector [i]);
	else
	   for (int i = 0; i < size; i ++)
	      v [i] = fftVector [i];
}

void	fftHandler::fft		(Complex  *v) {
	for (int i = 0; i < size; i ++)
	   fftVector [i] = v [i];
	fftwf_execute (plan);
	for (int i = 0;  i < size; i ++)
	   v [i] = fftVector [i];
}

