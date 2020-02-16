#
/*
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the dab library (dab-cmdline)
 *
 *    dab-cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation version 2 of the License.
 *
 *    dab-cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab-cmdline if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"sdrplay-handler-v3.h"
#include	"sdrplay-controller.h"


	sdrplayHandler_v3::sdrplayHandler_v3  (uint32_t	frequency,
	                                       int16_t	ppmCorrection,
	                                       int16_t	GRdB,
	                                       int16_t	lnaState,
	                                       bool	autogain,
	                                       uint16_t	deviceIndex,
	                                       int16_t	antenna) {
	this	-> vfoFrequency	= frequency;
	this	-> ppmCorrection	= ppmCorrection;
	this	-> GRdB		= GRdB;
	this	-> lnaState	= lnaState;
	this	-> autogain	= autogain;
	_I_Buffer		= new RingBuffer<
	                              std::complex<float>>(8 *1024 * 1024);
	theController	= new sdrplayController (frequency,
	                                         ppmCorrection,
	                                         GRdB,
	                                         lnaState,
	                                         autogain,
	                                         _I_Buffer);
}

	sdrplayHandler_v3::~sdrplayHandler_v3 () {
	delete theController;
	delete _I_Buffer;
}

bool	sdrplayHandler_v3::restartReader	(int32_t freq) {
	if (theController	-> isRunning ())
	   return true;
	theController	-> start ();
	return true;
}

void	sdrplayHandler_v3::stopReader	() {
	if (!theController -> isRunning ())
	   return;
	theController	-> stop ();
}

//	The brave old getSamples. For the sdrplay, we get
//	size still in I/Q pairs
//
int32_t	sdrplayHandler_v3::getSamples (std::complex<float> *V, int32_t size) { 
	return _I_Buffer     -> getDataFromBuffer (V, size);
}

int32_t	sdrplayHandler_v3::Samples	() {
	return _I_Buffer	-> GetRingBufferReadAvailable();
}

void	sdrplayHandler_v3::resetBuffer	() {
	_I_Buffer	-> FlushRingBuffer();
}

int16_t	sdrplayHandler_v3::bitDepth	() {
	return nrBits;
}

