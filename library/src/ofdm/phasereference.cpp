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
#include	"phasereference.h" 
#include	"string.h"
#include	"dab-params.h"
/**
  *	\class phaseReference
  *	Implements the correlation that is used to identify
  *	the "first" element (following the cyclic prefix) of
  *	the first non-null block of a frame
  *	The class inherits from the phaseTable.
  */
	phaseReference::phaseReference (dabParams	*p,
	                                int16_t		threshold):
	                                     phaseTable (p -> get_dabMode ()) {
int32_t	i;
float	Phi_k;

	this	-> T_u		= p -> get_T_u ();
	this	-> threshold	= threshold;

	Max			= 0.0;
	refTable		= new std::complex<float> 	[T_u];	//
	fft_processor		= new common_fft 	(T_u);
	fft_buffer		= fft_processor		-> getVector ();
	res_processor		= new common_ifft 	(T_u);
	res_buffer		= res_processor		-> getVector ();
	fft_counter		= 0;

	memset (refTable, 0, sizeof (std::complex<float>) * T_u);

	for (i = 1; i <= p -> get_carriers () / 2; i ++) {
	   Phi_k =  get_Phi (i);
	   refTable [i] = std::complex<float> (cos (Phi_k), sin (Phi_k));
	   Phi_k = get_Phi (-i);
	   refTable [T_u - i] = std::complex<float> (cos (Phi_k), sin (Phi_k));
	}
}

	phaseReference::~phaseReference (void) {
	delete	refTable;
	delete	fft_processor;
}

/**
  *	\brief findIndex
  *	the vector v contains "T_u" samples that are believed to
  *	belong to the first non-null block of a DAB frame.
  *	We correlate the data in this verctor with the predefined
  *	data, and if the maximum exceeds a threshold value,
  *	we believe that that indicates the first sample we were
  *	looking for.
  */
int32_t	phaseReference::findIndex (std::complex<float> *v) {
int32_t	i;
int32_t	maxIndex	= -1;
float	sum		= 0;

	Max	= 1.0;
	memcpy (fft_buffer, v, T_u * sizeof (std::complex<float>));

	fft_processor -> do_FFT ();
//
//	back into the frequency domain, now correlate
	for (i = 0; i < T_u; i ++) 
	   res_buffer [i] = fft_buffer [i] * conj (refTable [i]);
//	and, again, back into the time domain
	res_processor	-> do_IFFT ();
/**
  *	We compute the average signal value ...
  */
	for (i = 0; i < T_u; i ++)
	   sum	+= abs (res_buffer [i]);
	Max	= -10000;
	for (i = 0; i < T_u; i ++)
	   if (abs (res_buffer [i]) > Max) {
	      maxIndex = i;
	      Max = abs (res_buffer [i]);
	   }
/**
  *	that gives us a basis for defining the threshold
  */
	if (Max < threshold * sum / T_u) {
	   return  - abs (Max * T_u / sum) - 1;
	}
	else
	   return maxIndex;	
}
//
