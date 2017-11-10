#
/*
 *    Copyright (C)  2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB-cmdline program.
 *    Many of the ideas as implemented in DAB-cmdline are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    DAB-cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB-cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB-cmdline; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __DAB_CLASS__
#define __DAB_CLASS__

#include	"dab-constants.h"
#include	"dab-api.h"
#include	<vector>
#include	<string>
#include	<list>
#include	<atomic>
#include	<thread>

#include	"ofdm-processor.h"
#include	"fic-handler.h"
#include	"msc-handler.h"
#include	"ringbuffer.h"
#include	"ensemble-handler.h"
#include	"dab-params.h"

class	deviceHandler;

class dabClass {
public:
        	dabClass   (deviceHandler	*,
	                    uint8_t		Mode,
	                    RingBuffer<std::complex<float>> *spectrumBuffer,
	                    RingBuffer<std::complex<float>> *iqBuffer,
	                    syncsignal_t	syncsignalHandler,
	                    systemdata_t	systemdataHandler,
	                    ensemblename_t	ensemblenameHandler,
	                    programname_t	programnamehandler,
	                    fib_quality_t	fib_qualityHandler,
	                    audioOut_t		audioOut_Handler,
	                    dataOut_t		dataOut_Handler,
	                    bytesOut_t		bytesOut_Handler,
	                    programdata_t	programdataHandler,
	                    programQuality_t	program_qualityHandler,
	                    void		*ctx);
			~dabClass	(void);
	void		startProcessing	(void);
	void		reset		(void);
	void		stop		(void);
	std::string	dab_getserviceName (int32_t);
	int16_t		dab_service	(std::string);
	int32_t		dab_getSId	(std::string);
	bool		is_audioService	(std::string);
	bool		is_dataService	(std::string);
private:
	ficHandler	the_ficHandler;
	mscHandler	the_mscHandler;
	ofdmProcessor	*the_ofdmProcessor;
//

	deviceHandler		*inputDevice;
	RingBuffer<std::complex<float>>	*spectrumBuffer;
	RingBuffer<std::complex<float>>	*iqBuffer;
	syncsignal_t		syncsignalHandler;
	systemdata_t		systemdataHandler;
	ensemblename_t		ensemblenameHandler;
	programname_t		programnameHandler;
	fib_quality_t		fib_qualityHandler;
	audioOut_t		audioOut_Handler;
	dataOut_t		dataOut_Handler;
	programdata_t		programdataHandler;
	void			*userData;
};

#endif
