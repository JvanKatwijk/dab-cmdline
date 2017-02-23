#
/*
 *    Copyright (C) 2013
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
 *    along with DAB-library-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#
/*
 * 	MSC data
 */
#
#ifndef	MSC_HANDLER
#define	MSC_HANDLER

#include	<stdio.h>
#include	<stdint.h>
#include	<stdio.h>
#include	<mutex>
#include	<condition_variable>
#include	"dab-constants.h"
#include	"dab-api.h"

class	dabVirtual;

using namespace std;
class mscHandler {
public:
		mscHandler		(DabParams	*,
	                                 cb_audio_t,
	                                 cb_data_t);
		~mscHandler		(void);
	void	process_mscBlock	(int16_t *, int16_t);
	void	set_audioChannel	(audiodata	*);
	void	stopProcessing		(void);
	void	stopHandler		(void);
private:
	cb_audio_t	soundOut;
	cb_data_t	dataOut;
	bool		audioService;
	std::mutex	mutexer;
	dabVirtual	*dabHandler;
	int16_t		*cifVector;
	int16_t		cifCount;
	int16_t		blkCount;
	bool		work_to_be_done;
	bool		newChannel;
	int16_t		new_packetAddress;
	int16_t		new_ASCTy;
	int16_t		new_DSCTy;
	int16_t		new_startAddr;
	int16_t		new_Length;
	bool		new_shortForm;
	int16_t		new_protLevel;
	uint8_t		new_DGflag;
	int16_t		new_bitRate;
	int16_t		new_language;
	int16_t		new_type;
	int16_t		new_FEC_scheme;
	int16_t		startAddr;
	int16_t		Length;
	int8_t		dabModus;
	int8_t		new_dabModus;
	int16_t		BitsperBlock;
	int16_t		numberofblocksperCIF;
	int16_t		blockCount;
};

#endif


