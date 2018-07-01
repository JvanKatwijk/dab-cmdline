#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the  DAB-library
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
#ifndef	__OFDM_DECODER__
#define	__OFDM_DECODER__

#include	<stdint.h>
#include	<vector>
#include	<thread>
#include	<mutex>
#include	<condition_variable>
#include	<atomic>
#include	"dab-constants.h"
#include	"ringbuffer.h"
#include	"phasetable.h"
#include	"freq-interleaver.h"
#include	"fft_handler.h"
#include	"semaphore.h"

class	ficHandler;
class	mscHandler;
class	dabParams;

class	ofdmDecoder {
public:
		ofdmDecoder		(dabParams	*,
                                         RingBuffer<std::complex<float>> *,
	                                 ficHandler	*,
	                                 mscHandler	*);
		~ofdmDecoder		(void);
	void	processBlock_0		(std::complex<float> *);
	void	decodeBlock		(std::complex<float> *, int32_t n);
	int16_t	get_snr			(void);
	void	stop			(void);
	void	start			(void);
private:
	fft_handler	my_fftHandler;
	int16_t		get_snr		(std::complex<float> *);
	dabParams	*params;
        RingBuffer<std::complex<float>> *iqBuffer;
	ficHandler	*my_ficHandler;
	mscHandler	*my_mscHandler;
#ifdef	__THREADED_DECODING
	Semaphore	bufferSpace;
	std::thread	threadHandle;
	std::mutex	myMutex;
	std::mutex	ourMutex;
	std::condition_variable	Locker;
	void		run		(void);
	std::atomic<bool>	running;
	std::complex<float>	**command;
	int16_t		amount;
	int16_t		currentBlock;
	void		processBlock_0		(void);
	void		decodeFICblock		(int32_t n);
	void		decodeMscblock		(int32_t n);
#else
	void		decodeFICblock		(std::complex<float> *,
	                                                        int32_t n);
	void		decodeMscblock		(std::complex<float> *,
	                                                        int32_t n);
#endif
	int		cnt;
	int32_t		T_s;
	int32_t		T_u;
	int32_t		T_g;
	int32_t		carriers;
	int32_t		nrBlocks;
	int16_t		getMiddle	(void);
	std::vector <complex<float> >	phaseReference;
	std::vector<int16_t>            ibits;
	std::complex<float>	*fft_buffer;
	interLeaver	myMapper;
	int32_t		blockIndex;
	float		current_snr;
};

#endif


