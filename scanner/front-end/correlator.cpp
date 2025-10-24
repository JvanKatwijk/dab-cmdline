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
#include	"correlator.h" 
#include	<cstring>
#include	<vector>
/**
  *	\class correlator
  *	Implements the correlation that is used to identify
  *	the "first" element (following the cyclic prefix) of
  *	the first non-null block of a frame
  *	The class inherits from the phaseTable.
  */

	correlator::correlator (phaseTable 	*theTable) :
	                             fft_forward (params. get_T_u (), false),
	                             fft_backwards (params. get_T_u (), true) {
	                    

	this	-> theTable	= theTable;
	this	-> T_u		= params. get_T_u();
	this	-> T_g		= params. get_T_g();
	this	-> carriers	= params. get_carriers();
}

	correlator::~correlator () {
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
//
//	Pre v. size () >= T_u
//	Note that "v" is being modified by the fft function
//	that is why it is a value parameter
int32_t	correlator::findIndex (std::vector <Complex> v, int threshold) {
int32_t	maxIndex	= -1;
float	sum		= 0;
float	Max		= -1000;
float	lbuf [T_u / 2];

	fft_forward. fft (v);
//
//	into the frequency domain, now correlate
	for (int i = 0; i < T_u; i ++) 
	   v [i] = v [i] * Complex (real (theTable -> refTable [i]),
	                           -imag (theTable -> refTable [i]));

//	and, again, back into the time domain
	fft_backwards. fft (v);
/**
  *	We compute the average and the max signal values
  */
	for (int i = 0; i < T_u / 2; i ++) {
	   lbuf [i] = jan_abs (v [i]);
	   sum	+= lbuf [i];
	}

	sum /= T_u / 2;

	for (int i = 0; i < 50; i ++) {
           float absValue = abs (v [T_g - 40 + i]);
           if (absValue > Max) {
              maxIndex = T_g - 40 + i;
              Max = absValue;
           }
        }
/**
  *     that gives us a basis for validating the result
  */
        if (Max < threshold * sum) {
           return  - abs (Max / sum) - 1;
        }
        else
           return maxIndex;
}

