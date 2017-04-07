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
 *
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
#include	"phasereference.h"
#include	"ofdm-decoder.h"

#include	"virtual-input.h"
#include	"ringbuffer.h"
#include	"dab-api.h"

//
class	common_fft;
class	ofdmDecoder;
class	mscHandler;
class	ficHandler;
class	dabParams;

#define SEARCH_RANGE            (2 * 36)
#define CORRELATION_LENGTH      24

class ofdmProcessor {
public:
		ofdmProcessor  	(virtualInput *,
	                         dabParams	*,
	                         cb_system_data_t,
	                         mscHandler *,
	                         ficHandler *,
	                         int16_t,
	                         uint8_t);
	virtual ~ofdmProcessor	(void);
	void	reset			(void);
	void	stop			(void);
	void	setOffset		(int32_t);
	void	start			(void);
private:
	dabParams	*params;
	cb_system_data_t	systemData;
	void		call_systemData (bool, int16_t, int32_t);
	std::thread	threadHandle;
	int32_t		syncBufferIndex;
	std::atomic<bool>	running;
	virtualInput	*inputDevice;
	int32_t		T_null;
	int32_t		T_u;
	int32_t		T_s;
	int32_t		T_g;
	int32_t		T_F;
	int32_t		nrBlocks;
	int32_t		carriers;
	int32_t		carrierDiff;
	bool		isSynced;
	float		sLevel;
	DSPCOMPLEX	*dataBuffer;
	int32_t		FreqOffset;
	DSPCOMPLEX	*oscillatorTable;
	int32_t		localPhase;
	int16_t		fineCorrector;
	int32_t		coarseCorrector;

	uint8_t		freqsyncMethod;
	bool		f2Correction;
	DSPCOMPLEX	*ofdmBuffer;
	uint32_t	ofdmBufferIndex;
	uint32_t	ofdmSymbolCount;
	phaseReference	phaseSynchronizer;
	ofdmDecoder	my_ofdmDecoder;
	ficHandler	*my_ficHandler;
	mscHandler	*my_mscBuffer;

	float		correlationVector [CORRELATION_LENGTH + SEARCH_RANGE];
	float		refArg [CORRELATION_LENGTH];
	int32_t		sampleCnt;
	int32_t		inputSize;
	int32_t		inputPointer;
	DSPCOMPLEX	getSample	(int32_t);
	void		getSamples	(DSPCOMPLEX *, int16_t, int32_t);
virtual	void		run		(void);
	int32_t		bufferContent;
	bool		isReset;
	int16_t		processBlock_0	(DSPCOMPLEX *);
	int16_t		getMiddle	(DSPCOMPLEX *);
	common_fft	*fft_handler;
	DSPCOMPLEX	*fft_buffer;
};
#endif

