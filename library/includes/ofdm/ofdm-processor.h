#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the DAB-library program
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
#
#ifndef	__OFDM_PROCESSOR__
#define	__OFDM_PROCESSOR__
/*
 *
 */
#include	"dab-constants.h"
#include	<thread>
#include	<atomic>
#include	<stdint.h>
#include	<vector>
#include	"phasereference.h"
#include	"ofdm-decoder.h"
#include	"dab-params.h"
#include	"ringbuffer.h"
#include	"dab-api.h"
#include	"fft_handler.h"
#include	"sample-reader.h"
//
class	ofdmDecoder;
class	mscHandler;
class	ficHandler;
class	dabParams;
class	deviceHandler;

class ofdmProcessor {
public:
		ofdmProcessor  	(deviceHandler *,
	                         uint8_t,		// Mode
	                         syncsignal_t,
	                         systemdata_t,
	                         mscHandler *,
	                         ficHandler *,
	                         int16_t,
	                         RingBuffer<std::complex<float>> *,
                                 RingBuffer<std::complex<float>> *,
	                         void	*);
	virtual ~ofdmProcessor	(void);
	void	reset			(void);
	void	stop			(void);
	void	setOffset		(int32_t);
	void	start			(void);
	bool	signalSeemsGood		(void);
	void	show_Corrector		(int);
private:
	deviceHandler	*inputDevice;
	dabParams	params;
	syncsignal_t	syncsignalHandler;
	systemdata_t	systemdataHandler;
	void		call_systemData (bool, int16_t, int32_t);
	sampleReader	myReader;
	std::thread	threadHandle;
	void		*userData;
	std::atomic<bool>	running;
	bool		isSynced;
	int32_t		localPhase;
	int32_t		T_null;
	int32_t		T_u;
	int32_t		T_s;
	int32_t		T_g;
	int32_t		T_F;
	int32_t		nrBlocks;
	int32_t		carriers;
	int32_t		carrierDiff;
	float		sLevel;
	phaseReference	phaseSynchronizer;
	ofdmDecoder	my_ofdmDecoder;
	ficHandler	*my_ficHandler;
	mscHandler	*my_mscBuffer;

virtual	void		run		(void);
	bool		isReset;
        RingBuffer<std::complex<float>> *spectrumBuffer;
};
#endif

