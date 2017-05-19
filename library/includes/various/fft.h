#
/*
 *    Copyright (C) 2009 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the DAB-library.
 *    Many of the ideas as implemented in DAB-library are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
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
 */

#ifndef __COMMON_FFT__
#define	__COMMON_FFT__

#include	<stdint.h>
#include	<fftw3.h>
#include	<complex>
//
//	Simple wrapper around fftwf
//
#define	FFTW_MALLOC		fftwf_malloc
#define	FFTW_PLAN_DFT_1D	fftwf_plan_dft_1d
#define FFTW_DESTROY_PLAN	fftwf_destroy_plan
#define	FFTW_FREE		fftwf_free
#define	FFTW_PLAN		fftwf_plan
#define	FFTW_EXECUTE		fftwf_execute
/*
 *	a simple wrapper
 */
using namespace std;

class	common_fft {
public:
			common_fft	(int32_t);
			~common_fft	(void);
	std::complex<float>	*getVector	(void);
	void		do_FFT		(void);
	void		do_IFFT		(void);
	void		do_Shift	(void);
private:
	int32_t		fft_size;
	std::complex<float>	*vector;
	std::complex<float>	*vector1;
	FFTW_PLAN	plan;
	void		Scale		(std::complex<float> *);
};

class	common_ifft {
public:
			common_ifft	(int32_t);
			~common_ifft	(void);
	std::complex<float>	*getVector	(void);
	void		do_IFFT		(void);
private:
	int32_t		fft_size;
	std::complex<float>	*vector;
	FFTW_PLAN	plan;
	void		Scale		(std::complex<float> *);
};

#endif

