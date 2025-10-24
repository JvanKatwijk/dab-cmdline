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
#
#include	"sample-reader.h"
#include	"device-handler.h"
#include	"dab-processor.h"

static
Complex oscillatorTable [SAMPLE_RATE];

	sampleReader::sampleReader (dabProcessor *parent,
	                            deviceHandler	*theRig) {
	theParent		= parent;
	this	-> theRig	= theRig;
	bufferSize		= 32768;
	currentPhase		= 0;
	sLevel			= 0;
	sampleCount		= 0;
	for (int i = 0; i < SAMPLE_RATE; i ++)
	   oscillatorTable [i] = Complex
	                            (cos (2.0 * M_PI * i / SAMPLE_RATE),
	                             sin (2.0 * M_PI * i / SAMPLE_RATE));

	corrector	= 0;
	running. store (true);
}

	sampleReader::~sampleReader () {
}

void	sampleReader::reset	() {
	currentPhase            = 0;
	sLevel                  = 0;
	sampleCount             = 0;
}


void	sampleReader::setRunning (bool b) {
	running. store (b);
}

float	sampleReader::get_sLevel () {
	return sLevel;
}

Complex sampleReader::getSample (int32_t phaseOffset) {
Complex temp;

	if (!running. load ())
	   throw 21;

	while (running. load () && (theRig -> Samples () < 1))
	      usleep (100);

	if (!running. load ())	
	   throw 20;
//
//	so here, bufferContent > 0
	theRig -> getSamples (&temp, 1);

//	OK, we have a sample!!
//	first: adjust frequency. We need Hz accuracy
	currentPhase	-= phaseOffset;
	currentPhase	= (currentPhase + SAMPLE_RATE) % SAMPLE_RATE;

	temp		*= oscillatorTable [currentPhase];
	sLevel		= 0.00001 * jan_abs (temp) + (1 - 0.00001) * sLevel;
#define	N	5
	return temp;
}

void	sampleReader::getSamples (Complex *v, int32_t n, int32_t phaseOffset) {
int32_t		i;

	while (running. load () && (theRig -> Samples () < n))
	   usleep (100);

	if (!running. load ())	
	   throw 20;
//
	n = theRig -> getSamples (v, n);

//	OK, we have samples!!
//	first: adjust frequency. We need Hz accuracy
	for (i = 0; i < n; i ++) {
	   currentPhase	-= phaseOffset;
//
//	Note that "phase" itself might be negative
	   currentPhase	= (currentPhase + SAMPLE_RATE) % SAMPLE_RATE;
	   v [i]	*= oscillatorTable [currentPhase];
	   sLevel	= 0.00001 * jan_abs (v [i]) + (1 - 0.00001) * sLevel;
	}

	sampleCount	+= n;
}

