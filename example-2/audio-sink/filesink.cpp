#
/*
 *    Copyright (C)  2009, 2010, 2011
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library
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
 *
 */
#include	<stdio.h>
#include	"filesink.h"

	fileSink::fileSink	(std::string fileName, bool *success) {
SF_INFO sf_info;

	sf_info. samplerate	= 48000;
	sf_info. channels	= 2;
	sf_info. format	= SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	fprintf (stderr, "trying to open %s\n", fileName. c_str ());
	outputFile		= sf_open (fileName. c_str (),
	                                           SFM_WRITE, &sf_info);
	if (outputFile == nullptr) {
	   fprintf (stderr, "cannot open %s\n", fileName. c_str ());
	   *success	= false;
	   return;
	}
	fprintf (stderr, "Opened %s\n", fileName. c_str ());
	*success	= true;
	audioOK		= *success;
}

	fileSink::~fileSink	() {
	   if (audioOK)
	      sf_close (outputFile);
}

void	fileSink::stop		() {
}

void	fileSink::restart	() {
}

void	fileSink::audioOutput	(float *buffer, int32_t amount) {
	if (audioOK) {
	   int16_t localBuffer [2 * amount];
	   int16_t i;
	   for (i = 0; i < amount; i ++) {
	      localBuffer [2 * i    ] = buffer [2 * i] * 32767;
	      localBuffer [2 * i + 1] = buffer [2 * i + 1] * 32767;
	   }
	   sf_write_short (outputFile, localBuffer, amount);
	}
}


