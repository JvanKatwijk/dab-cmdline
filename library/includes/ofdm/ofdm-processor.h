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
#include	"phasereference.h"
#include	"ofdm-decoder.h"
#include	"dab-params.h"
#include	"ringbuffer.h"
#include	"dab-api.h"

//
class	common_fft;
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
	                         uint8_t,
	                         RingBuffer<std::complex<float>> *,
                                 RingBuffer<std::complex<float>> *,
	                         void	*);
	virtual ~ofdmProcessor	(void);
	void	reset			(void);
	void	stop			(void);
	void	setOffset		(int32_t);
	void	start			(void);
	bool	signalSeemsGood		(void);
private:
	deviceHandler	*inputDevice;
	dabParams	params;
	syncsignal_t	syncsignalHandler;
	systemdata_t	systemdataHandler;
	void		call_systemData (bool, int16_t, int32_t);
	std::thread	threadHandle;
	int32_t		syncBufferIndex;
	void		*userData;
	std::atomic<bool>	running;
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
	std::complex<float>	*dataBuffer;
	int32_t		FreqOffset;
	std::complex<float>	*oscillatorTable;
	int32_t		localPhase;
	int16_t		fineCorrector;
	int32_t		coarseCorrector;

	uint8_t		freqsyncMethod;
	bool		f2Correction;
	std::complex<float>	*ofdmBuffer;
	uint32_t	ofdmBufferIndex;
	uint32_t	ofdmSymbolCount;
	phaseReference	phaseSynchronizer;
	ofdmDecoder	my_ofdmDecoder;
	ficHandler	*my_ficHandler;
	mscHandler	*my_mscBuffer;
	bool		tiiSwitch;

	int32_t		sampleCnt;
	int32_t		inputSize;
	int32_t		inputPointer;
	std::complex<float>	getSample	(int32_t);
	void		getSamples	(std::complex<float> *, int16_t, int32_t);
virtual	void		run		(void);
	int32_t		bufferContent;
	bool		isReset;
	int16_t		getMiddle	(std::complex<float> *);
	common_fft	*fft_handler;
	std::complex<float>	*fft_buffer;
        RingBuffer<std::complex<float>> *spectrumBuffer;
        int32_t         bufferSize;
        int32_t         localCounter;
        std::complex<float>      *localBuffer;
};
#endif

