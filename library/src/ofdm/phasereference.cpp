#
/*
 *    Copyright (C) 2016, 2017
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
	phaseReference::phaseReference (uint8_t	dabMode,
	                                int16_t	diff_length):
	                                     phaseTable (dabMode),
	                                     params (dabMode),
	                                     my_fftHandler (dabMode) {
int32_t	i;
float	Phi_k;
        this    -> T_u          = params. get_T_u ();
        this    -> T_g          = params. get_T_g ();
        this    -> diff_length  = diff_length;
        refTable.               resize (T_u);
        phaseDifferences.       resize (diff_length);
        fft_buffer              = my_fftHandler. getVector ();

        for (i = 1; i <= params. get_carriers () / 2; i ++) {
           Phi_k =  get_Phi (i);
           refTable [i] = std::complex<float> (cos (Phi_k), sin (Phi_k));
           Phi_k = get_Phi (-i);
           refTable [T_u - i] = std::complex<float> (cos (Phi_k), sin (Phi_k));
        }
//
//      prepare a table for the coarse frequency synchronization
	shiftFactor	= this -> diff_length / 4;
        for (i = 0; i < diff_length; i ++) {
           phaseDifferences [i] =
	                abs (arg (refTable [(T_u - shiftFactor + i) % T_u] *
                                  conj (refTable [(T_u - shiftFactor + i + 1) % T_u])));
	   phaseDifferences [i] *= phaseDifferences [i];
	}
}

	phaseReference::~phaseReference (void) {
}

/**
  *	\brief findIndex
  *	the vector v contains "T_u" samples that are believed to
  *	belong to the first non-null block of a DAB frame.
  *	We correlate the data in this vector with the predefined
  *	data, and if the maximum exceeds a threshold value,
  *	we believe that that indicates the first sample we were
  *	looking for.
  */
int32_t	phaseReference::findIndex (std::complex<float> *v, int threshold) {
int32_t	i;
int32_t	maxIndex	= -1;
float	sum		= 0;
float	Max		= -10000;

	memcpy (fft_buffer, v, T_u * sizeof (std::complex<float>));
	my_fftHandler. do_FFT ();

//	into the frequency domain, now correlate
	for (i = 0; i < T_u; i ++) 
	   fft_buffer [i] *= conj (refTable [i]);
//	and, again, back into the time domain
	my_fftHandler. do_iFFT ();
/**
  *	We compute the average signal value ...
  */
	for (i = 0; i < T_u / 2; i ++) 
	   sum += abs (fft_buffer [i]);

	sum /= (T_u / 2);

	for (i = 0; i < 50; i ++) {
	   float absValue = abs (fft_buffer [T_g - 40 + i]);
	   if (absValue > Max) {
	      maxIndex = T_g - 40 + i;
	      Max = absValue;
	   }
	}
/**
  *	that gives us a basis for validating the result
  */
	if (Max < threshold * sum) {
	   return  - abs (Max / sum) - 1;
	}
	else
	   return maxIndex;	
}

#define SEARCH_RANGE    (2 * 35)
int16_t phaseReference::estimateOffset (std::complex<float> *v) {
int16_t i, j, index_1 = 100, index_2 = 100;
#ifdef _MSC_VER
float   *computedDiffs = (float *)_alloca((SEARCH_RANGE + diff_length + 1) * sizeof(float));
#else
float   computedDiffs [SEARCH_RANGE + diff_length + 1];
#endif

	for (i = 0; i < T_u; i ++)
	   fft_buffer [i] = v [i];

	my_fftHandler. do_FFT ();

	for (i = T_u - SEARCH_RANGE / 2;
	     i < T_u + SEARCH_RANGE / 2 + diff_length; i ++) 
	   computedDiffs [i - (T_u - SEARCH_RANGE / 2)] =
	      arg (fft_buffer [(i - shiftFactor) % T_u] *
	           conj (fft_buffer [(i - shiftFactor + 1) % T_u]));

	for (i = 0; i < SEARCH_RANGE + diff_length; i ++)
	   computedDiffs [i] *= computedDiffs [i];

        float   Mmin = 10000;
        float   Mmax = 0;

	for (i = T_u - SEARCH_RANGE / 2;
             i < T_u + SEARCH_RANGE / 2; i ++) {
           int sum_1 = 0;
           int sum_2 = 0;
           for (j = 0; j < diff_length; j ++) {
              if (phaseDifferences [j] < 0.05)
                 sum_1 += computedDiffs [i - (T_u - SEARCH_RANGE / 2) + j];
	      if (phaseDifferences [j] > M_PI - 0.05)
                 sum_2 += computedDiffs [i - (T_u - SEARCH_RANGE / 2) + j];
           }
           if (sum_1 < Mmin) {
              Mmin = sum_1;
              index_1 = i;
           }
           if (sum_2 > Mmax) {
              Mmax = sum_2;
              index_2 = i;
           }
        }

        return index_1 == index_2 ? index_1 - T_u : 100;
}

