#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library of the SDR-J software
 *    DAB library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#
#ifndef	__DATA_PROCESSOR__
#define	__DATA_PROCESSOR__

#include	<stdio.h>
#include	<string.h>
#include	<vector>
#include	"dab-api.h"
#include	"backend-base.h"
#include    "reed-solomon.h"


class	virtual_dataHandler;

class	dataProcessor:public backendBase {
public:
	dataProcessor	(int16_t	bitRate,
	                 packetdata	*pd,
	                 API_struct	*p,
	                 void		*ctx);
	~dataProcessor	(void);
void	addtoFrame	(uint8_t *);
private:
	int16_t		bitRate;
	uint8_t		DSCTy;
	int16_t		appType;
	int16_t		packetAddress;
	uint8_t		DGflag;
	int16_t		FEC_scheme;
	bytesOut_t	bytesOut;
    programQuality_t        mscQuality; // taken from mp4processor.h
	void		*ctx;
	int16_t		crcErrors;
	int16_t     frameCount;
    int16_t     frameErrors;
    int16_t     rsErrors;
    int16_t     frame_quality;
    int16_t     rs_quality;
	uint8_t		pdlen; 				// to check that was passed from the API

	std::vector<uint8_t> series;
	uint8_t		packetState;
	std::vector<uint8_t> ByteBuf;

	int16_t		blockFillIndex;
    int16_t     blocksInBuffer;
    uint8_t		curMSC;
    uint8_t		curPI;
	std::vector<uint8_t> frameBytes;
	std::vector<uint8_t> outVector; // added for RS decoding
	uint8_t		RSDims;
    reedSolomon my_rsDecoder;

//
//	result handlers
	void		handleTDCAsyncstream (uint8_t *, int16_t);
	void		handlePackets		 (uint8_t *, int16_t);
	void		handleRSdata		 (uint8_t *);						// handle RS data bytes
	uint8_t		processRSData		 (uint8_t *, uint16_t);			 	// process RS data bytes
	void		handleAppData		 (uint8_t *, int16_t);				// handle application data
	void		applyFEC			 (void);
    void 		verifyRSdecoder		 (uint8_t* , uint8_t* , uint8_t*); 	// verification of RS decoder
	void 		processOutVector	 (const std::vector<uint8_t>&);
	void		processPacketStream	 (void);
	virtual_dataHandler *my_dataHandler;
};

#endif

