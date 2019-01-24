#
/*
 *    Copyright (C) 2008 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the dab library
 *
 *    dab library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dab library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	"fft_handler.h"
#include	<cstring>

	fft_handler::fft_handler (uint8_t dabMode): p (dabMode) {
	int i;
	this	-> fftSize	= p. get_T_u ();
	vector	= (complex<float> *)
	                fftwf_malloc (sizeof (complex<float>) * fftSize);
	for (i = 0; i < fftSize; i ++)
	   vector [i] = std::complex<float> (0, 0);
	plan	= fftwf_plan_dft_1d (fftSize,
	                            reinterpret_cast <fftwf_complex *>(vector),
	                            reinterpret_cast <fftwf_complex *>(vector),
	                            FFTW_FORWARD, FFTW_ESTIMATE);
}

	fft_handler::~fft_handler (void) {
	   fftwf_destroy_plan (plan);
	   fftwf_free (vector);
}

complex<float>	*fft_handler::getVector () {
	return vector;
}
//
void	fft_handler::do_FFT (void) {
	fftwf_execute (plan);
}

//	Note that we do not scale in case of backwards fft,
//	not needed for our applications
void	fft_handler::do_iFFT (void) {
int	i;
	for (i = 0; i < fftSize; i ++)
	   vector [i] = conj (vector [i]);
	fftwf_execute (plan);
	for (i = 0; i < fftSize; i ++)
	   vector [i] = conj (vector [i]);
}

