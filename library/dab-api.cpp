#
/*
 *    Copyright (C) 2016, 2025
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
	            API_struct		*theParameters,
	            RingBuffer<std::complex<float>> *spectrumBuffer,
	            RingBuffer<std::complex<float>> *iqBuffer,
	            void                *userData) {
dabProcessor *theClass = new dabProcessor (theDevice,
	                                   theParameters,
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

bool	is_audioService	(void *Handle, const std::string &name) {
	return ((dabProcessor *)Handle) -> serviceType (name) ==
	                               AUDIO_SERVICE;
}

bool	is_dataService	(void *Handle, const std::string &name) {
	return ((dabProcessor *)Handle) -> serviceType (name) ==
	                               PACKET_SERVICE;
}

void    dataforAudioService     (void *Handle, const std::string &name,
	                                     audiodata &d, int o) {
	((dabProcessor *)Handle) -> dataforAudioService (name, d, o);
}

void    dataforDataService      (void *Handle, const std::string &name,
	                                     packetdata &pd, int o) {
	((dabProcessor *)Handle) -> dataforDataService (name, pd, o);
}

void    set_audioChannel        (void *Handle, audiodata &ad) {
	((dabProcessor *)Handle) -> set_audioChannel (ad);
}

void    set_dataChannel         (void *Handle, packetdata &pd) {
	((dabProcessor *)Handle) -> set_dataChannel (pd);
}

int32_t dab_getSId      (void *Handle, const std::string &c_s) {
	return ((dabProcessor *)Handle) -> get_SId (c_s);
}
//
std::string	dab_getserviceName (void *Handle, uint32_t SId) {
	return ((dabProcessor *)Handle) -> get_serviceName (SId);
	
}

std::string	get_ensembleName	(void *Handle) {
	return ((dabProcessor *)Handle) -> get_ensembleName	();
}

#ifdef _MSC_VER
#include <windows.h>
extern "C" {

void	usleep(int usec) {
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10*usec); // Convert to 100 nanosecond interval, negative value indicates relative time

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
}

void sleep(int seconds) {
    Sleep (seconds * 1000);
}
}

#endif

