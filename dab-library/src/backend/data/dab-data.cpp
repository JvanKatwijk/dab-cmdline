/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J (JSDR).
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#
#include	"dab-constants.h"
#include	"dab-data.h"
#include	"dab-processor.h"
#include	"eep-protection.h"
#include	"uep-protection.h"
#include	"data-processor.h"
#include	<chrono>

//
//	fragmentsize == Length * CUSize
	dabData::dabData	(uint8_t DSCTy,
	                       	 int16_t packetAddress,
	                         int16_t fragmentSize,
	                         int16_t bitRate,
	                       	 bool	shortForm,
	                         int16_t protLevel,
	                         uint8_t DGflag,
	                       	 int16_t FEC_scheme,
	                         bool	show_crcErrors) {
int32_t i;
	this	-> DSCTy		= DSCTy;
	this	-> packetAddress	= packetAddress;
	this	-> fragmentSize		= fragmentSize;
	this	-> bitRate		= bitRate;
	this	-> shortForm		= shortForm;
	this	-> protLevel		= protLevel;
	this	-> DGflag		= DGflag;
	this	-> FEC_scheme		= FEC_scheme;
	this	-> show_crcErrors	= show_crcErrors;
	our_dabProcessor	= new dataProcessor (bitRate,
	                                             DSCTy,
	                                             DGflag,
	                                             FEC_scheme,
	                                             show_crcErrors);
	outV			= new uint8_t [bitRate * 24];
	interleaveData		= new int16_t *[16]; // the size
	for (i = 0; i < 16; i ++) {
	   interleaveData [i] = new int16_t [fragmentSize];
	   memset (interleaveData [i], 0, fragmentSize * sizeof (int16_t));
	}
	countforInterleaver	= 0;
//
//	The handling of the depuncturing and deconvolution is
//	shared with that of the audio
	if (shortForm)
	   protectionHandler	= new uep_protection (bitRate,
	                                              protLevel);
	else
	   protectionHandler	= new eep_protection (bitRate,
	                                              protLevel);
//
//	any reasonable (i.e. large) size will do here,
//	as long as the parameter is a power of 2
	Buffer		= new RingBuffer<int16_t>(64 * 32768);
	start ();
}

	dabData::~dabData	(void) {
int16_t	i;
	running = false;
	threadHandle. join ();
	delete Buffer;
	delete protectionHandler;
	delete[]	outV;
	for (i = 0; i < 16; i ++)
	   delete[] interleaveData [i];
	delete[]	interleaveData;
}

void    dabData::start         (void) {
        threadHandle = std::thread (&dabData::run, this);
}


int32_t	dabData::process	(int16_t *v, int16_t cnt) {
int32_t	fr;
	   while ((fr = Buffer -> GetRingBufferWriteAvailable ()) < cnt) {
	      if (!running)
	         return 0;
	      usleep (10);
	   }
	   Buffer	-> putDataIntoBuffer (v, cnt);
	   Locker. notify_all ();
	   return fr;
}

const   int16_t interleaveMap[] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};

void	dabData::run	(void) {
int16_t	countforInterleaver	= 0;
int16_t interleaverIndex	= 0;
uint8_t	shiftRegister [9];
int16_t	Data [fragmentSize];
int16_t	i, j;

	running	= true;
	while (running) {
	   while (Buffer -> GetRingBufferReadAvailable () <= fragmentSize) {
              std::unique_lock <std::mutex> lck (ourMutex);
              auto now = std::chrono::system_clock::now ();
              Locker. wait_until (lck, now + std::chrono::milliseconds (1));      // 1 msec waiting time
              if (!running)
                 break;
           }

	   if (!running) 
	      break;

	   Buffer	-> getDataFromBuffer (Data, fragmentSize);
//
	   for (i = 0; i < fragmentSize; i ++) {
	      interleaveData [interleaverIndex][i] = Data [i];
	      Data [i] = interleaveData [(interleaverIndex + 
	                                 interleaveMap [i & 017]) & 017][i];
	   }
	   interleaverIndex = (interleaverIndex + 1) & 0x0F;

//	only continue when de-interleaver is filled
	   if (countforInterleaver <= 15) {
	      countforInterleaver ++;
	      continue;
	   }
//
	   protectionHandler -> deconvolve (Data, fragmentSize, outV);
//
//	and the inline energy dispersal
	   memset (shiftRegister, 1, 9);
	   for (i = 0; i < bitRate * 24; i ++) {
	      uint8_t b = shiftRegister [8] ^ shiftRegister [4];
	      for (j = 8; j > 0; j--)
	         shiftRegister [j] = shiftRegister [j - 1];
	      shiftRegister [0] = b;
	      outV [i] ^= b;
	   }
//	What we get here is a long sequence (24 * bitrate) of bits, not packed
//	but forming a DAB packet
//	we hand it over to make an MSC data group
	   our_dabProcessor -> addtoFrame (outV);
	}
}

//	It might take a msec for the task to stop
void	dabData::stopRunning (void) {
	running = false;
	threadHandle. join ();
}
//
