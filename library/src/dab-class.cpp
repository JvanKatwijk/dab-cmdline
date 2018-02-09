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
	                    uint8_t		Mode,
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
	                                   the_ficHandler (ensemblename_Handler,
	                                                   programname_Handler,
	                                                   fibquality_Handler,
	                                                   ctx),
	                                   the_mscHandler (Mode,
	                                                   audioOut_Handler,
	                                                   dataOut_Handler,
	                                                   bytesOut_Handler,
	                                                   programquality_Handler,
	                                                   motdata_Handler,
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

	 the_ofdmProcessor = new ofdmProcessor  (inputDevice,
	                                        Mode,
	                                        syncsignal_Handler,
	                                        systemdata_Handler,
	                                        &the_mscHandler,
	                                        &the_ficHandler,
	                                        3,
	                                        spectrumBuffer,
	                                        iqBuffer,
	                                        userData);
}

	dabClass::~dabClass	(void) {
	the_ofdmProcessor	-> stop ();
	delete the_ofdmProcessor;
}

void	dabClass::startProcessing	(void) {
	fprintf (stderr, "ofdm word gestart\n");
	the_ofdmProcessor	-> start ();
}

void	dabClass::reset		(void) {
	the_ofdmProcessor	-> reset ();	
	the_ficHandler. clearEnsemble ();
}

void	dabClass::stop		(void) {
	the_ofdmProcessor	-> stop ();	
	the_ficHandler. clearEnsemble ();
}
//
//	The return value > 0, then success, 
//	otherwise error

bool	dabClass::is_audioService (std::string name) {
	switch (the_ficHandler. kindofService (name)) {
	   case AUDIO_SERVICE:
	      return true;
	   default:
	      return false;
	}
}

bool	dabClass::is_dataService (std::string name) {
	switch (the_ficHandler. kindofService (name)) {
	   case PACKET_SERVICE:
	      return true;
	   default:
	      return false;
	}
}

int16_t	dabClass::dab_service (std::string name) {
int	componentNumber	= 0;	// default main service
audiodata d1;
packetdata d2;
std::string searchName	= name;
int	k	= the_ficHandler. kindofService (name);
	if (searchName [0] == '*') {
	   searchName. erase (0, 1);	
	   componentNumber = 1;
	}

	switch (k) {
	   case AUDIO_SERVICE:
	     the_ficHandler. dataforAudioService (searchName,
	                                          &d1, componentNumber);
	     if (!d1. defined) 
	        return -4;
	     the_mscHandler. set_audioChannel (&d1);
	     fprintf (stderr, "selected %s\n", name. c_str ());
	     if (programdata_Handler != NULL) 
	        programdata_Handler (&d1, userData);
	     return 1;
//
//	For the command line version, we do not have a data service
	   case PACKET_SERVICE:
	      the_ficHandler. dataforDataService (searchName,
	                                          &d2, componentNumber);
	      if (!d2. defined)
	         return -3;
	      the_mscHandler. set_dataChannel (&d2);
	      fprintf (stderr, " selected service %s\n", name. c_str ());
	      return 1;

	   default:
	      return -2;
	}
	return -1;
}

int32_t	dabClass::dab_getSId	(std::string name) {
	return the_ficHandler. SIdFor (name);
}

std::string dabClass::dab_getserviceName (int32_t SId) {
	return the_ficHandler. nameFor (SId);
}

