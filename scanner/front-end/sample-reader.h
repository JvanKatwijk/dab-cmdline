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
#pragma once
/*
 *	Reading the samples from the input device. Since it has its own
 *	"state", we embed it into its own class
 */
#include	"dab-constants.h"
#include	<stdint.h>
#include	<atomic>
#include	<vector>
#include	"ringbuffer.h"
//

class	deviceHandler;
class	dabProcessor;

class	sampleReader {
public:
			sampleReader	(dabProcessor *, deviceHandler *theRig);
			~sampleReader		();
		void	setRunning	(bool b);
		float	get_sLevel	();
	        void	reset		();
		Complex getSample	(int32_t);
	        void	getSamples	(Complex *v,
	                                 int32_t n, int32_t phase);
private:
		dabProcessor	*theParent;
		deviceHandler	*theRig;
		int32_t		bufferSize;
		int32_t		currentPhase;
		std::atomic<bool>	running;
		float		sLevel;
		int32_t		sampleCount;
	        int32_t		corrector;
};

