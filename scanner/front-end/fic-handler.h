#
/*
 *    Copyright (C) 2025
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the 2-nd DAB scannner and borrowed
 *    for the Qt-DAB repository of the same author
 *
 *    DAB scannner is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB scanner is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB scanner; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#
/*
 * 	FIC data
 */
#pragma once

#include	<cstdio>
#include	<cstdint>
#include	<vector>
#include	<atomic>
#include	<mutex>
#include	"viterbi-spiral.h"
#include	"dab-params.h"
#include	"fib-decoder.h"
#include	"dab-api.h"

class	dabParams;

class ficHandler: public fibDecoder {
public:
		ficHandler		(API_struct *, void *);
		~ficHandler		();
	void	processFICBlock		(std::vector<int16_t> &, int16_t);
	void	stop			();
	void	restart			();
	void	getFIBBits		(uint8_t *, bool *);
	int	getFICQuality		();
private:
	dabParams	params;
	viterbiSpiral	myViterbi;
	uint8_t		hardBits	[768];
        int16_t		ficInput	[2304];
	uint8_t		punctureTable	[3072 + 24];
	uint8_t		fibBits		[4 * 768];
	bool		ficValid	[4];
	uint8_t		PRBS		[768];
	bool		processFICInput	(int16_t);
	int16_t		index;
	int16_t		BitsperBlock;
	int16_t		ficno;
	uint16_t	convState;
	int		ficBlocks;
	int		ficErrors;
	int		ficBits;
	int		ficPointer;
	std::atomic<bool> running;
	int		successRatio;
	int		fibCounter;
	uint16_t	starter;
};

