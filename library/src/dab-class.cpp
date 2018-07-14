#
/*
 *    Copyright (C) 2015, 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of DAB-library II
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
 *    along with DAB-library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	<unistd.h>
#include	<termios.h>
#include	<stdio.h>
#include	<sys/select.h>
#include	<atomic>
#include	"dab-constants.h"
#include	"dab-class.h"
#include	"device-handler.h"
//
//	dabclass is a convenience class
	dabClass::dabClass (deviceHandler	*inputDevice,
	                    uint8_t		dabMode,
	                    RingBuffer<std::complex<float>> *spectrumBuffer,
	                    RingBuffer<std::complex<float>> *iqBuffer,
	                    syncsignal_t	syncsignal_Handler,
	                    systemdata_t	systemdata_Handler,
	                    ensemblename_t	ensemblename_Handler,
	                    programname_t	programname_Handler,
	                    fib_quality_t	fibquality_Handler,
	                    audioOut_t		audioOut_Handler,
	                    dataOut_t		dataOut_Handler,
	                    bytesOut_t		bytesOut_Handler,
	                    programdata_t	programdata_Handler,
	                    programQuality_t	programquality_Handler,
	                    motdata_t		motdata_Handler,
	                    void		*ctx):
	                               the_dabProcessor (inputDevice,
	                                                 dabMode,
	                                                 syncsignal_Handler,
	                                                 systemdata_Handler,
	                                                 ensemblename_Handler,
	                                                 programname_Handler,
	                                                 fibquality_Handler,
	                                                 audioOut_Handler,
	                                                 bytesOut_Handler,
	                                                 dataOut_Handler,
	                                                 programdata_Handler,
	                                                 programquality_Handler,
	                                                 motdata_Handler,
	                                                 spectrumBuffer,
	                                                 iqBuffer,
	                                                 ctx) {
	this	-> inputDevice		= inputDevice;
	this	-> spectrumBuffer	= spectrumBuffer,
	this	-> iqBuffer		= iqBuffer,
	this	-> syncsignal_Handler	= syncsignal_Handler;
	this	-> systemdata_Handler	= systemdata_Handler;
	this	-> ensemblename_Handler	= ensemblename_Handler;
	this	-> programname_Handler	= programname_Handler;
	this	-> fibquality_Handler	= fibquality_Handler;
	this	-> audioOut_Handler	= audioOut_Handler;
	this	-> dataOut_Handler	= dataOut_Handler;
	this	-> programdata_Handler	= programdata_Handler;
	this	-> motdata_Handler	= motdata_Handler;
	this	-> userData		= ctx;
}

	dabClass::~dabClass	(void) {
	the_dabProcessor. stop ();
}

void	dabClass::startProcessing	(void) {
	the_dabProcessor. start ();
}

void	dabClass::reset		(void) {
	the_dabProcessor. reset ();	
}

void	dabClass::stop		(void) {
	the_dabProcessor. stop ();	
}
//
bool	dabClass::is_audioService (std::string name) {
	return the_dabProcessor. kindofService (name) == AUDIO_SERVICE;
}

bool	dabClass::is_dataService (std::string name) {
	return the_dabProcessor. kindofService (name) == PACKET_SERVICE;
}
//
//	result > 0 success
int16_t	dabClass::dab_service (std::string name) {
int	k	= the_dabProcessor. kindofService (name);
	the_dabProcessor. reset_msc ();	// vital here
	switch (k) {
	   case AUDIO_SERVICE: {
	      audiodata ad;
	      int i;
	      the_dabProcessor. dataforAudioService (name, &ad, 0);
	      if (!ad. defined) 
	         return -4;
	      the_dabProcessor. set_audioChannel (&ad);
	      if (programdata_Handler != nullptr) 
	         programdata_Handler (&ad, userData);

	      for (i = 1; i < 5; i ++) {
	         packetdata pd;
	         the_dabProcessor. dataforDataService (name, &pd, i);
	         if (pd. defined) {
	            the_dabProcessor. set_dataChannel (&pd);
	            break;
	         }
	      }
	     
	      return 1;
	   }

	   case PACKET_SERVICE: {
	      packetdata d2;
	      the_dabProcessor. dataforDataService (name, &d2, 0);
	      if (!d2. defined)
	         return -3;
	      the_dabProcessor. set_dataChannel (&d2);
	      return 1;
	   }

	   default:
	      return -2;
	}
	return -1;
}

int32_t	dabClass::dab_getSId (std::string s) {
	return the_dabProcessor. get_SId (s);
}

std::string dabClass::dab_getserviceName (int32_t SId) {
	return the_dabProcessor. get_serviceName (SId);
}

