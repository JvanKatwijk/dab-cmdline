#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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
	            syncsignal_t        syncsignal_Handler,
	            systemdata_t        systemdata_Handler,
	            ensemblename_t      ensemblename_Handler,
	            programname_t       programname_Handler,
	            fib_quality_t       fib_quality_Handler,
	            audioOut_t          audioOut_Handler,
	            dataOut_t           dataOut_Handler,
	            bytesOut_t		bytesOut_Handler,
	            programdata_t       programdata_Handler,
	            programQuality_t    programquality_Handler,
	            motdata_t		motdata_Handler,
	            void                *userData) {
dabClass *theClass = new dabClass (theDevice,
	                           Mode,
	                           spectrumBuffer,
	                           iqBuffer,
	                           syncsignal_Handler,
	                           systemdata_Handler,
	                           ensemblename_Handler,
	                           programname_Handler,
	                           fib_quality_Handler,
	                           audioOut_Handler,
	                           dataOut_Handler,
	                           bytesOut_Handler,
	                           programdata_Handler,
	                           programquality_Handler,
	                           motdata_Handler,
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

int16_t	dabService     (const char* c_s, void *Handle) {
	std::string s(c_s);
	return ((dabClass *)Handle) -> dab_service (s);
}
	
int32_t dab_getSId      (const char* c_s, void * Handle) {
	std::string s(c_s);
	return ((dabClass *)Handle) -> dab_getSId (s);
}

std::string dab_getserviceName (int32_t SId, void *Handle) {
	return ((dabClass *)Handle) -> dab_getserviceName (SId);
}

