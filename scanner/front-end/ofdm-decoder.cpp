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
#include	"ofdm-decoder.h"
#include	"phasetable.h"
#include	"freq-interleaver.h"
#include	"dab-params.h"

#define	ALPHA 0.005f
static inline
float	compute_avg (float a, float b, float c) {
	return (1.0f - c) * a + c * b;
}

static inline
Complex normalize (const Complex &V) {
float length	= abs (V);
	if (length < 0.001)
	   return Complex (0, 0);
	return V / length;
}

static inline
void	limit_symmetrically (float &in, float ref) {
	if (in >= ref)
	   in = ref;
	if (in <= -ref)
	   in = -ref;
}

#define	MAX_VITERBI	127

	ofdmDecoder::ofdmDecoder	():
	                               myMapper    (),
	                               my_fftHandler (params. get_T_u (),
	                                                              false),		                               phaseReference (params. get_T_u ()),
	                               fft_buffer (params. get_T_u ()),
	                               meanLevelVector (params. get_T_u ()),
	                               sigmaSQ_Vector (params. get_T_u ()) {
	                               


	this	-> T_s			= params. get_T_s ();
	this	-> T_u			= params. get_T_u ();
	this	-> nrBlocks		= params. get_L ();
	this	-> carriers		= params. get_carriers ();
	this	-> T_g			= T_s - T_u;
	reset	();
	cnt				= 0;
	sqrt_2				= sqrt (2) / 2.0;
}

	ofdmDecoder::~ofdmDecoder	() {
}

void	ofdmDecoder::reset	() {
	for (int i = 0; i < T_u; i ++) {
	   meanLevelVector [i]	= 0;
	   sigmaSQ_Vector [i]	= 0;
	}
	meanValue	= 1.0;
}

void	ofdmDecoder::processBlock_0 (std::vector<Complex> buffer) {
	my_fftHandler. fft (buffer);
/**
  *	we are now in the frequency domain, and we keep the carriers
  *	as coming from the FFT as phase reference.
  */
	memcpy (phaseReference. data (),
	                    buffer. data (), T_u * sizeof (Complex));
}

void	ofdmDecoder::decode (std::vector<Complex>  &buffer, int32_t blkno,
	                     std::vector<int16_t> &ibits, float snr) {

float	sum = 0;
	memcpy (fft_buffer. data (), &((buffer. data ()) [T_g]), 
                                       T_u * sizeof (Complex));
	my_fftHandler. fft (fft_buffer. data ());
/**
  *	a little optimization: we do not interchange the
  *	positive/negative frequencies to their right positions.
  *	The de-interleaving understands this
  */
//toBitsLabel:
/**
  *	Note that from here on, we are only interested in the
  *	"carriers" useful carriers of the FFT output
  */
	for (int i = 0; i < carriers; i ++) {
	   int16_t	index	= myMapper. mapIn (i);
	   if (index < 0) 
	      index += T_u;
	   Complex current	= fft_buffer [index];
	   Complex prevS	= phaseReference [index];
	   Complex fftBin	= current * normalize (conj (prevS));
	   float binAbsLevel	= abs (fftBin);

	   std::complex<float> fftBin_at_1 =
	                 std::complex<float> (abs (real (fftBin)),
	                                             abs (imag (fftBin)));
	   meanLevelVector [index]	=
	                 compute_avg (meanLevelVector [index],
	                                         binAbsLevel, ALPHA);
	   float d_x			= abs (real (fftBin_at_1)) -
	                                       meanLevelVector [index] / sqrt_2;
	   float d_y			= abs (imag (fftBin_at_1)) -
	                                       meanLevelVector [index] / sqrt_2;
	   float sigmaSQ		= d_x * d_x + d_y * d_y;
	   sigmaSQ_Vector [index]	=
	                 compute_avg (sigmaSQ_Vector [index], sigmaSQ, ALPHA);
//
//	Those were the preparations, now for real
	   float corrector		= 
	                 1.5 * meanLevelVector [index] / sigmaSQ_Vector [index];
	   corrector			/= (1 / snr + 2);
	   std::complex<float> R1	=
	                 corrector * normalize(fftBin) *
	                    (float)(sqrt (binAbsLevel *
	                                  abs (phaseReference [index])));
	   float scaler		= 140.0 / meanValue;
	   float leftBit	= - real (R1) * scaler;
	   limit_symmetrically (leftBit, MAX_VITERBI);
           ibits [i]		= (int16_t)leftBit;

	   float  rightBit	= - imag (R1) * scaler;
           limit_symmetrically (rightBit, MAX_VITERBI);
           ibits [i + carriers]   = (int16_t)rightBit;
	   sum                       += abs (R1);
	}
	meanValue	= compute_avg (meanValue, sum / carriers, 0.1);
	memcpy (phaseReference. data (),
	          fft_buffer. data (), T_u * sizeof (Complex));
}

