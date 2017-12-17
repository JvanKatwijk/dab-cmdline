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
	                    syncsignal_t	syncsignalHandler,
	                    systemdata_t	systemdataHandler,
	                    ensemblename_t	ensemblenameHandler,
	                    programname_t	programnameHandler,
	                    fib_quality_t	fib_qualityHandler,
	                    audioOut_t		audioOut_Handler,
	                    dataOut_t		dataOut_Handler,
	                    bytesOut_t		bytesOut_Handler,
	                    programdata_t	programdataHandler,
	                    programQuality_t	program_qualityHandler,
	                    void		*ctx):
	                                   the_ficHandler (ensemblenameHandler,
	                                                   programnameHandler,
	                                                   fib_qualityHandler,
	                                                   ctx),
	                                   the_mscHandler (Mode,
	                                                   audioOut_Handler,
	                                                   dataOut_Handler,
	                                                   bytesOut_Handler,
	                                                   program_qualityHandler,
	                                                   ctx) {
	this	-> inputDevice		= inputDevice;
	this	-> spectrumBuffer	= spectrumBuffer,
	this	-> iqBuffer		= iqBuffer,
	this	-> syncsignalHandler	= syncsignalHandler;
	this	-> systemdataHandler	= systemdataHandler;
	this	-> ensemblenameHandler	= ensemblenameHandler;
	this	-> programnameHandler	= programnameHandler;
	this	-> fib_qualityHandler	= fib_qualityHandler;
	this	-> audioOut_Handler	= audioOut_Handler;
	this	-> dataOut_Handler	= dataOut_Handler;
	this	-> programdataHandler	= programdataHandler;
	this	-> userData		= ctx;

	 the_ofdmProcessor = new ofdmProcessor  (inputDevice,
	                                        Mode,
	                                        syncsignalHandler,
	                                        systemdataHandler,
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
audiodata d1;
packetdata d2;

	switch (the_ficHandler. kindofService (name)) {
	   case AUDIO_SERVICE:
	     the_ficHandler. dataforAudioService (name, &d1);
	     if (!d1. defined) 
	        return -4;
	     the_mscHandler. set_audioChannel (&d1);
	     fprintf (stderr, "selected %s\n", name. c_str ());
	     if (programdataHandler != NULL) 
	        programdataHandler (&d1, userData);
	     return 1;
//
//	For the command line version, we do not have a data service
	   case PACKET_SERVICE:
	      the_ficHandler. dataforDataService (name, &d2);
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

