#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is the C/C++implementation of the DAB-API
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
 *    along with DAB-library, if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
//
#include	"dab-api.h"
#include	"ringbuffer.h"
#include	"dab-class.h"

void	*dabInit   (deviceHandler       *theDevice,
	            uint8_t             Mode,
	            RingBuffer<std::complex<float>> *spectrumBuffer,
	            RingBuffer<std::complex<float>> *iqBuffer,
	            syncsignal_t        syncsignalHandler,
	            systemdata_t        systemdataHandler,
	            ensemblename_t      ensemblenameHandler,
	            programname_t       programnameHandler,
	            fib_quality_t       fib_qualityHandler,
	            audioOut_t          audioOut_Handler,
	            dataOut_t           dataOut_Handler,
	            programdata_t       programdataHandler,
	            programQuality_t    program_qualityHandler,
	            void                *userData) {
dabClass *theClass = new dabClass (theDevice,
	                           Mode,
	                           spectrumBuffer,
	                           iqBuffer,
	                           syncsignalHandler,
	                           systemdataHandler,
	                           ensemblenameHandler,
	                           programnameHandler,
	                           fib_qualityHandler,
	                           audioOut_Handler,
	                           dataOut_Handler,
	                           programdataHandler,
	                           program_qualityHandler,
	                           userData);
	return (void *)theClass;
}

void	dabExit		(void *Handle) {
	delete (dabClass *)Handle;
}

void	dabStartProcessing (void *Handle) {
	((dabClass *)Handle) -> startProcessing ();
}

void	dabReset           (void *Handle) {
	((dabClass *)Handle) -> reset ();
}


void	dabStop           (void *Handle) {
	((dabClass *)Handle) -> stop ();
}


int16_t	dabService     (std::string s, void *Handle) {
	return ((dabClass *)Handle) -> dab_service (s);
}
	
int32_t dab_getSId      (std::string s, void * Handle) {
	return ((dabClass *)Handle) -> dab_getSId (s);
}

std::string dab_getserviceName (int32_t SId, void *Handle) {
	return ((dabClass *)Handle) -> dab_getserviceName (SId);
}

