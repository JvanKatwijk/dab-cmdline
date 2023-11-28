#
/*
 *    Copyright (C) 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the main program for the DAB library
 *
 *    DAB library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"audio-base.h"
#include	<stdio.h>

/*
 *	The class is the abstract sink for the data generated
 *	It will handle the "dumping" though
 */
	audioBase::audioBase	():
	                              converter_16 (16000, 48000, 2 * 1600),
	                              converter_24 (24000, 48000, 2 * 2400),
	                              converter_32 (32000, 48000, 4 * 3200) {
}

	audioBase::~audioBase	() {
}

void	audioBase::restart	() {
}

void	audioBase::stop		() {
}
//
//	This one is a hack for handling different baudrates coming from
//	the aac decoder. call is from the GUI, triggered by the
//	aac decoder or the mp2 decoder
void	audioBase::audioOut	(int16_t *buffer,
	                         int32_t amount, int32_t rate) {
	switch (rate) {
	   case 16000:	
	      audioOut_16000 (buffer, amount / 2);
	      return;

	   case 24000:
	      audioOut_24000 (buffer, amount / 2);
	      return;

	   case 32000:
	      audioOut_32000 (buffer, amount / 2);
	      return;

	   default:
	   case 48000:
	      audioOut_48000 (buffer, amount / 2);
	      return;
	}
}
//
//	scale up from 16 -> 48
//	amount gives number of pairs
void	audioBase::audioOut_16000	(int16_t *V, int32_t amount) {
DSPCOMPLEX outputBuffer [converter_16. getOutputsize ()];
float      buffer       [2 * converter_16. getOutputsize ()];
int16_t	i, j;
int32_t	result;

	for (i = 0; i < amount; i ++)
	   if (converter_16. convert (DSPCOMPLEX (V [2 * i] / 32767.0,
	                                          V [2 * i + 1] / 32767.0),
	                              outputBuffer, &result)) {
	      for (j = 0; j < result; j ++) {
	         buffer [2 * j    ] = real (outputBuffer [j]);
	         buffer [2 * j + 1] = imag (outputBuffer [j]);
	      }
	   
	      audioOutput (buffer, result);
	   }
}

//	scale up from 24000 -> 48000
//	amount gives number of pairs
void	audioBase::audioOut_24000	(int16_t *V, int32_t amount) {
DSPCOMPLEX outputBuffer [converter_24. getOutputsize ()];
float      buffer       [2 * converter_24. getOutputsize ()];
int16_t	i, j;
int32_t	result;

	for (i = 0; i < amount; i ++)
	   if (converter_24. convert (DSPCOMPLEX (V [2 * i] / 32767.0,
	                                          V [2 * i + 1] / 32767.0),
	                              outputBuffer, &result)) {
	      for (j = 0; j < result; j ++) {
	         buffer [2 * j    ] = real (outputBuffer [j]);
	         buffer [2 * j + 1] = imag (outputBuffer [j]);
	      }
	   
	      audioOutput (buffer, result);
	   }
}
//
//	scale up from 32000 -> 48000
//	amount is number of pairs
void	audioBase::audioOut_32000	(int16_t *V, int32_t amount) {
DSPCOMPLEX outputBuffer [converter_32. getOutputsize ()];
float      buffer       [2 * converter_32. getOutputsize ()];
int32_t	i, j;
int32_t	result;

	for (i = 0; i < amount; i ++) {
	   if (converter_32. convert (DSPCOMPLEX (V [2 * i] / 32767.0,
	                                          V [2 * i + 1] / 32767.0),
	                              outputBuffer, &result)) {
	      for (j = 0; j < result; j ++) {
	         buffer [2 * j    ] = real (outputBuffer [j]);
	         buffer [2 * j + 1] = imag (outputBuffer [j]);
	      }
	   
	      audioOutput (buffer, result);
	   }
	}
}

void	audioBase::audioOut_48000	(int16_t *V, int32_t amount) {
float *buffer = (float *)alloca (2 * amount * sizeof (float));
int32_t	i;

	for (i = 0; i < amount; i ++) {
	   buffer [2 * i]	= V [2 * i] / 32767.0;
	   buffer [2 * i + 1]	= V [2 * i + 1] / 32767.0;
	}

	audioOutput (buffer, amount);
}
//
//	The audioOut function is the one that really should be
//	reimplemented in the offsprings of this class
void	audioBase::audioOutput	(float *v, int32_t amount) {
	(void)v;
	(void)amount;
}

