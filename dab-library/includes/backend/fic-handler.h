#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the DAB-library
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
/*
 * 	FIC data
 */
#ifndef	__FIC_HANDLER
#define	__FIC_HANDLER

#include	<stdio.h>
#include	<stdint.h>
#include	"viterbi.h"
#include	"fib-processor.h"
#include	<mutex>
#include	<string>

class	ensembleHandler;

class ficHandler: public viterbi {
public:
		ficHandler		(ensembleHandler *);
		~ficHandler		(void);
	void	process_ficBlock	(int16_t *, int16_t);
	void	setBitsperBlock		(int16_t);
	void	clearEnsemble		(void);
	bool	syncReached		(void);
	int16_t	get_ficRatio		(void);
	std::string nameFor		(int32_t);
	uint8_t	kindofService		(std::string &);
	void	dataforDataService	(std::string &, packetdata *);
	void	dataforAudioService	(std::string &, audiodata *);
private:
	void		process_ficInput	(int16_t *, int16_t);
	int8_t		*PI_15;
	int8_t		*PI_16;
	uint8_t		*bitBuffer_in;
	uint8_t		*bitBuffer_out;
	int16_t		*ofdm_input;
	int16_t		index;
	int16_t		BitsperBlock;
	int16_t		ficno;
	int16_t		ficBlocks;
	int16_t		ficMissed;
	int16_t		ficRatio;
	uint16_t	convState;
	mutex		fibProtector;
	fib_processor	fibProcessor;
	uint8_t		PRBS [768];
	uint8_t		shiftRegister [9];
};

#endif


