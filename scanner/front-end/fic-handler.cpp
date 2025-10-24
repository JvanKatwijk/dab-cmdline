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

#include	"fic-handler.h"
#include	"crc-handlers.h"
#include	"protTables.h"
#include	"dab-params.h"
#include	"dab-api.h"

//	From 2304 incoming (soft) bits, a "motherword" of 3072 bits
//	is formed by depuncturing with predefined puncture tables.
//	The 3072 bits of the serial motherword shall be split into
//	24 blocks of 128 bits each.
//	The first 21 blocks shall be subjected to
//	puncturing (per 32 bits) according to PI_16
//	The next three blocks shall be subjected to 
//	puncturing (per 32 bits) according to PI_15
//	The last 24 bits shall be subjected to puncturing
//	according to the table 8

#define	FIC_BLOCKSIZE	3072
#define	FIC_RESIDU	24
#define	FIC_INPUT	2304
/**
  *	\class ficHandler
  * 	We get in - through process_ficBlock - the FIC data
  * 	in units of 768 bits (i.e. FIC_BLOCKSIZE / 4)
  * 	We follow the standard and apply convolution decoding and
  * 	puncturing.
  *	The data is sent through to the fib processor
  */

		ficHandler::ficHandler (API_struct *p,  void *userData):
	                                    fibDecoder (p, userData),
	                                    myViterbi (768) {
int16_t	shiftRegister [9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};

	BitsperBlock	= 2 * params. get_carriers();
	index		= 0;
	ficno		= 0;
	ficBlocks	= 0;

	starter		= 0;
	ficErrors	= 0;
	ficBits		= 0;

	for (int i = 0; i < FIC_BLOCKSIZE / 4; i ++) {
	   PRBS [i] = shiftRegister [8] ^ shiftRegister [4];
	   for (int j = 8; j > 0; j --)
	      shiftRegister [j] = shiftRegister [j - 1];

	   shiftRegister [0] = PRBS [i];
	}
//
//	Since the depuncturing is the same throughout all calls
//	(even through all instances, so we could create a static
//	table), we make an punctureTable that contains the indices of
//	the ficInput table
	memset (punctureTable, (uint8_t)false,
	                       (FIC_BLOCKSIZE + FIC_RESIDU) * sizeof (uint8_t));
	int	local	= 0;
	for (int i = 0; i < 21; i ++) {
	   for (int k = 0; k < 32 * 4; k ++) {
	      if (get_PCodes (16 - 1) [k % 32] != 0)  
	         punctureTable [local] = true;
	      local ++;
	   }
	}
/**
  *	In the second step
  *	we have 3 blocks with puncturing according to PI_15
  *	each 128 bit block contains 4 subblocks of 32 bits
  *	on which the given puncturing is applied
  */
	for (int i = 0; i < 3; i ++) {
	   for (int k = 0; k < 32 * 4; k ++) {
	      if (get_PCodes (15 - 1) [k % 32] != 0)  
	         punctureTable [local] = true;
	      local ++;
	   }
	}
/**
  *	we have a final block of 24 bits  with puncturing according to PI_X
  *	This block constitues the 6 * 4 bits of the register itself.
  */
	for (int k = 0; k < 24; k ++) {
	   if (get_PCodes (8 - 1) [k] != 0) 
	      punctureTable [local] = true;
	   local ++;
	}

	ficPointer	= 0;
	fibCounter	= 1;
	successRatio	= 0;
}

		ficHandler::~ficHandler () {
}
	
/**
  *	\brief process_ficBlock
  *	The number of bits to be processed per incoming block
  *	is 2 * p -> K, which still depends on the Mode.
  *	for Mode I it is 2 * 1536, for Mode II, it is 2 * 384,
  *	for Mode III it is 192, Mode IV gives 2 * 768.
  *	for Mode II we will get the 2304 bits after having read
  *	the 3 FIC blocks, each with 768 bits.
  *	for Mode IV we will get 3 * 2 * 768 = 4608, i.e. two resulting blocks
  *	Note that Mode III is NOT supported
  *	
  *	The function is called with a blkno. This should be 1, 2 or 3
  *	for each time 2304 bits are in, we call process_ficInput
  */
//
//	pre data. size () >= BitsperBlock
void	ficHandler::processFICBlock (std::vector<int16_t> &softBits,
	                                                 int16_t blkno) {
	if (blkno == 1) {
	   index = 0;
	   ficno = 0;
	}

	if (starter == 0) {
	   if (blkno != 1) {
	      return;
	   }
	}
	if (starter < 6) {
	   starter ++;
	   return;
	}

	if ((1 <= blkno) && (blkno <= 3)) {
	   for (int i = 0; i < BitsperBlock; i ++) {
	      ficInput [index ++] = softBits [i];
	      if (index >= FIC_INPUT) {
	         ficValid [ficno] = processFICInput (ficno);
	         index = 0;
	         ficno ++;
	      }
	   }
	}
	else
	   fprintf (stderr, "You should not call ficBlock here\n");
//	we are pretty sure now that after block 4, we end up
//	with index = 0
}

/**
  *	\brief process_ficInput
  *	we have a vector of 2304 (0 .. 2303) soft bits that has
  *	to be de-punctured and de-conv-ed into a block of 768 bits
  *	In this approach we first create the full 3072 block (i.e.
  *	we first depuncture, and then we apply the deconvolution
  *	In the next coding step, we will combine this function with the
  *	one above
  */
bool	ficHandler::processFICInput (int16_t ficno) {
static
int16_t	viterbiInput [FIC_BLOCKSIZE + FIC_RESIDU] = {0};
int16_t	inputCount	= 0;

	for (int i = 0; i < FIC_BLOCKSIZE + FIC_RESIDU; i ++)  {
	   viterbiInput [i] =  punctureTable [i] ?
	                       ficInput [inputCount ++] : 0;
	}
/**
  *	Now we have the full word ready for deconvolution
  *	deconvolution is according to DAB standard section 11.2
  */
	myViterbi. deconvolve (viterbiInput, hardBits);
/**
  *	if everything worked as planned, we now have a
  *	768 bit vector containing three FIB's (fib blocks) with "hard" bits
  *
  *	first step: energy dispersal according to the DAB standard
  *	We use a predefined vector PRBS
  */
	for (int i = 0; i < FIC_BLOCKSIZE / 4; i ++) {
	   hardBits [i] ^= PRBS [i];
	   fibBits [ficno * FIC_BLOCKSIZE / 4 + i] = hardBits [i];
	}
/**
  *	each of the 3 fib blocks is protected by a crc
  *	(we know that there are three fib blocks each time we are here)
  *	we keep track of the successrate and show that per 100 fic blocks
  *	One issue is what to do when we really believe the synchronization
  *	was lost.
  */
	static int teller = 0;
	for (int i = ficno * 3; i < ficno * 3 + 3; i ++) {
	   teller ++;
	   uint8_t *ficBlock = &hardBits [(i % 3) * 256];
	   if (!check_CRC_bits (ficBlock, 256)) {
	      continue;
	   }
	   fibDecoder::process_FIB (ficBlock, ficno);
	}
	if (teller > 200)
	   teller = 0;
	return true;
}

void	ficHandler::stop	() {
	disconnectChannel	();
	running. store (false);
}

void	ficHandler::restart	() {
//	clearEnsemble	();
	index		= 0;
	ficno		= 0;
	ficBlocks	= 0;

	ficErrors	= 0;
	ficBits		= 0;
	starter		= 0;
	connectChannel	();
	running. store (true);
}

void	ficHandler::getFIBBits		(uint8_t *v, bool *b) {
	for (int i = 0; i < 4 * 768; i ++)
	   v [i] = fibBits [i];
	for (int i = 0; i < 4; i ++)
	    b [i] = ficValid [i];
}

int	ficHandler::getFICQuality	() {
	return 0;
}
