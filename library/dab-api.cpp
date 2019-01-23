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
#include	"dab-processor.h"

void	*dabInit   (deviceHandler       *theDevice,
	            uint8_t             Mode,
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
	            RingBuffer<std::complex<float>> *spectrumBuffer,
	            RingBuffer<std::complex<float>> *iqBuffer,
	            void                *userData) {
dabProcessor *theClass = new dabProcessor (theDevice,
	                                   Mode,
	                                   syncsignal_Handler,
	                                   systemdata_Handler,
	                                   ensemblename_Handler,
	                                   programname_Handler,
	                                   fib_quality_Handler,
	                                   audioOut_Handler,
	                                   bytesOut_Handler,
	                                   dataOut_Handler,
	                                   programdata_Handler,
	                                   programquality_Handler,
	                                   motdata_Handler,
	                                   spectrumBuffer,
	                                   iqBuffer,
	                                   userData);
	return (void *)theClass;
}

void	dabExit		(void *Handle) {
	delete (dabProcessor *)Handle;
}

void	dabStartProcessing (void *Handle) {
	((dabProcessor *)Handle) -> start ();
}

void	dabReset	(void *Handle) {
	((dabProcessor *)Handle) -> reset ();
}

void	dabStop		(void *Handle) {
	((dabProcessor *)Handle) -> stop ();
}

void	dabReset_msc	(void *Handle) {
	((dabProcessor *)Handle) -> reset_msc ();
}

bool	is_audioService	(void *Handle, const char *name) {
	return ((dabProcessor *)Handle) -> kindofService (std::string (name)) ==
	                               AUDIO_SERVICE;
}

bool	is_dataService	(void *Handle, const char *name) {
	return ((dabProcessor *)Handle) -> kindofService (std::string (name)) ==
	                               PACKET_SERVICE;
}

void	dab_setTII_handler(void *Handle, tii_t tii_Handler, tii_ex_t tii_ExHandler, int tii_framedelay, float alfa, int resetFrameCount) {
	return ((dabProcessor *)Handle) -> setTII_handler (tii_Handler, tii_ExHandler, tii_framedelay, alfa, resetFrameCount);
}

//	functions contributed by Hayati Ayguen
//	mainId < 0 (-1) => don't check mainId
//	subId == -1 => deliver first available offset
//	subId == -2 => deliver coarse coordinates
void	dab_getCoordinates	(void *Handle,
	                         int16_t mainId, int16_t subId,
	                         float *latitude, float *longitude,
	                         bool *success,
	                         int16_t *pMainId, int16_t *pSubId,
	                         int16_t *pTD) {
        std::complex<float> r =
	      ((dabProcessor *)Handle) -> get_coordinates (mainId,
	                                                   subId,
	                                                   success,
	                                                   pMainId,
	                                                   pSubId, pTD);
        *latitude = r.real();
        *longitude = r.imag();
}

uint8_t	dab_getExtendedCountryCode	(void *Handle,
	                                 bool *success) {
	return ((dabProcessor *)Handle) -> getECC(success);
}

uint8_t dab_getInternationalTabId	(void *Handle,
	                                 bool *success) {
	return ((dabProcessor *)Handle) -> getInterTabId(success);
}

void    dataforAudioService     (void *Handle,
	                         const char *name,
	                         audiodata *d, int o) {
	((dabProcessor *)Handle) -> dataforAudioService (name, d, o);
}

void    dataforDataService      (void *Handle,
	                         const char *name, packetdata *pd, int o) {
	((dabProcessor *)Handle) -> dataforDataService (name, pd, o);
}

void    set_audioChannel        (void *Handle, audiodata *ad) {
	((dabProcessor *)Handle) -> set_audioChannel (ad);
}

void    set_dataChannel         (void *Handle, packetdata *pd) {
	((dabProcessor *)Handle) -> set_dataChannel (pd);
}

int32_t dab_getSId      (void *Handle, const char* c_s) {
	std::string s(c_s);
	return ((dabProcessor *)Handle) -> get_SId (s);
}

std::string dab_getserviceName (void *Handle, int32_t SId) {
	return ((dabProcessor *)Handle) -> get_serviceName (SId);
}

