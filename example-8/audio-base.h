#
/*
 *    Copyright (C)  2009, 2010, 2011
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

#ifndef __AUDIO_BASE__
#define	__AUDIO_BASE__
#include	<stdio.h>
#include	<samplerate.h>
#include	"newconverter.h"
#include	"ringbuffer.h"

typedef float   DSPFLOAT;
typedef std::complex<DSPFLOAT> DSPCOMPLEX;

using namespace std;


class	audioBase {
public:
			audioBase		(void);
virtual			~audioBase		(void);
virtual	void		stop			(void);
virtual	void		restart			(void);
	void		audioOut		(int16_t *, int32_t, int32_t);
private:
	void		audioOut_16000		(int16_t *, int32_t);
	void		audioOut_24000		(int16_t *, int32_t);
	void		audioOut_32000		(int16_t *, int32_t);
	void		audioOut_48000		(int16_t *, int32_t);
	newConverter	converter_16;
	newConverter	converter_24;
	newConverter	converter_32;
protected:
virtual	void		audioOutput		(float *, int32_t);
};
#endif

