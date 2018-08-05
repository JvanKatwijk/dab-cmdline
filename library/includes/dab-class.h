
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
#include        <complex>
#include	<list>
#include	<atomic>
#include	<thread>

#include	"dab-processor.h"
#include	"ringbuffer.h"
#include	"ensemble-handler.h"
#include	"dab-params.h"

class	deviceHandler;

class dabClass {
public:
        	dabClass   (deviceHandler	*,
	                    uint8_t		dabMode,
	                    RingBuffer<std::complex<float>> *spectrumBuffer,
	                    RingBuffer<std::complex<float>> *iqBuffer,
	                    syncsignal_t	syncsignal_Handler,
	                    systemdata_t	systemdata_Handler,
	                    ensemblename_t	ensemblename_Handler,
	                    programname_t	programname_handler,
	                    fib_quality_t	fibquality_Handler,
	                    audioOut_t		audioOut_Handler,
	                    dataOut_t		dataOut_Handler,
	                    bytesOut_t		bytesOut_Handler,
	                    programdata_t	programdata_Handler,
                            programPacketData_t programPacketData_Handler,
                            packetdata_t        packetdata_Handler,
	                    programQuality_t	programquality_Handler,
	                    motdata_t		motdata_Handler,
	                    void		*ctx);
			~dabClass	(void);
	void		startProcessing	(void);
	void		reset		(void);
	void		stop		(void);
	int16_t		dab_service	(std::string, bool callbackDataOnly = false);
	int32_t		dab_getSId	(std::string);
	std::string	dab_getserviceName (int32_t);
	bool		is_audioService	(std::string);
	bool		is_dataService	(std::string);

	void		dab_getCoordinates(int16_t mainId, int16_t subId, float *latitude, float *longitude, bool *success);
	uint8_t		dab_getExtendedCountryCode(bool *success);

private:
	uint8_t		dabMode;
	dabProcessor	the_dabProcessor;
//

	deviceHandler		*inputDevice;
	RingBuffer<std::complex<float>>	*spectrumBuffer;
	RingBuffer<std::complex<float>>	*iqBuffer;
	syncsignal_t		syncsignal_Handler;
	systemdata_t		systemdata_Handler;
	ensemblename_t		ensemblename_Handler;
	programname_t		programname_Handler;
	fib_quality_t		fibquality_Handler;
	audioOut_t		audioOut_Handler;
	dataOut_t		dataOut_Handler;
	programdata_t		programdata_Handler;
        programPacketData_t     programPacketData_Handler;
        packetdata_t            packetdata_Handler;
	motdata_t		motdata_Handler;
	void			*userData;
};

#endif
