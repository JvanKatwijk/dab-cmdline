#
/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library 
 *
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
#pragma once

#include	<stdio.h>
#include	<string.h>
#include	<vector>
#include	<atomic>
#include	"dab-api.h"
#include	"backend-base.h"
#include	"reed-solomon.h"


class	virtual_dataHandler;

class	dataProcessor:public backendBase {
public:
	dataProcessor	(int16_t	bitRate,
	                 packetdata	*pd,
	                 API_struct	*p,
	                 void		*ctx);
	~dataProcessor	();
void	addtoFrame	(uint8_t *);
private:
	int16_t		bitRate;
	uint8_t		DSCTy;
	int16_t		appType;
	int16_t		packetAddress;
	uint8_t		DGflag;
	int16_t		FEC_scheme;
	int16_t		expectedIndex;
	std::vector<uint8_t>	series;
	int16_t		fillPointer;
	bool		assembling;
	std::vector<uint8_t> AppVector;
	std::vector<uint8_t> FECVector;
	bool		FEC_table [9];
	reedSolomon my_rsDecoder;
	bytesOut_t	bytesOut;
	int32_t		streamAddress;		// int since we init with -1
	std::atomic<bool>	running;
//
//	result handlers
	void		handleTDCAsyncstream 	(uint8_t *, int32_t);
	void		handlePackets		(uint8_t *, int16_t);
	void		handlePacket		(uint8_t *vec);

	void		handleRSPacket		(uint8_t *);
	void		registerFEC		(uint8_t *, int);
	void		clear_FECtable		();
	bool		FEC_complete		();
	void		handle_RSpackets	(std::vector<uint8_t> &);
	void		handle_RSpacket		(uint8_t *, int16_t);
	int		addPacket		(uint8_t *, 
	                                         std::vector<uint8_t> &, int);

	void		processRS		(std::vector<uint8_t> &appdata,
                                  	         const std::vector<uint8_t> &RSdata);
	virtual_dataHandler *my_dataHandler;
};


