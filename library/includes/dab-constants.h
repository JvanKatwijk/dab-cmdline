#
/*
 *    Copyright (C) 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library
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
//
//	Common definitions and includes for
//	the DAB decoder

#pragma once
#
#include	<math.h>
#include	<stdint.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<complex>
#include	<limits>
#include	<cstring>
#ifndef _MSC_VER
#include	<unistd.h>
#endif

#ifndef __APPLE__
#ifndef	__FREEBSD__
#include	<malloc.h>
#endif
#endif

#if defined(__MINGW32__) || defined(_MSC_VER)
//#include	"iostream.h"
#include	"windows.h"
#ifdef _MSC_VER
#include	<malloc.h>
#undef min
#undef max
extern "C" {
void sleep	(int);
void usleep	(int usec);
}
#endif
#else
#ifndef	__FREEBSD__
#include	"alloca.h"
#endif
#include	"dlfcn.h"
typedef	void	*HINSTANCE;
#endif

using namespace std;
//
#define	Hz(x)		(x)
#define	Khz(x)		(x * 1000)
#define	KHz(x)		(x * 1000)
#define	kHz(x)		(x * 1000)
#define	Mhz(x)		(Khz (x) * 1000)
#define	MHz(x)		(KHz (x) * 1000)
#define	mHz(x)		(kHz (x) * 1000)

#define	CURRENT_VERSION	"2.0alfa"

#define		DAB		0100
#define		DAB_PLUS	0101

#define		AUDIO_SERVICE	0101
#define		PACKET_SERVICE	0102
#define		UNKNOWN_SERVICE	0100

#define		INPUT_RATE	2048000
#define		BANDWIDTH	1536000

#define		SYNCED		01
#define		UNSYNCED	04

#define		DIFF_LENGTH	42
#define		THRESHOLD	3

static inline
bool	isIndeterminate (float x) {
	return x != x;
}

static inline
bool	isInfinite (double x) {
	return x == numeric_limits<float>::infinity ();
}

static inline
std::complex<float> cmul (std::complex<float> x, float y) {
	return std::complex<float> (real (x) * y, imag (x) * y);
}

static inline
std::complex<float> cdiv (std::complex<float> x, float y) {
	return std::complex<float> (real (x) / y, imag (x) / y);
}

static inline
float	get_db (float x) {
	return 20 * log10 ((x + 1) / (float)(256));
}
//

//
//	A simple approximation of the abs value, just for checking
//	values during time synchronization
static inline
float	jan_abs (std::complex<float> z) {
float	re	= real (z);
float	im	= imag (z);
	if (re < 0) re = - re;
	if (im < 0) im = - im;
	if (re > im) 
           return re + 0.5 * im;
        else
           return im + 0.5 * re; 
}
//

//	These are defined elsewhere
////	for service handling we define
//typedef struct {
//	int16_t subchId;
//	int16_t	startAddr;
//	bool	shortForm;
//	int16_t	protLevel;
//	int16_t DSCTy;
//	int16_t	length;
//	int16_t	bitRate;
//	int16_t	FEC_scheme;
//	int16_t	DGflag;
//	int16_t	packetAddress;
//} packetdata;
//
//typedef	struct {
//	int16_t	subchId;
//	int16_t	startAddr;
//	bool	shortForm;
//	int16_t	protLevel;
//	int16_t	length;
//	int16_t	bitRate;
//	int16_t	ASCTy;
//	int16_t	language;
//	int16_t	programType;
//	bool	defined;
//} audiodata;
//
//	just some locals
//
//	generic, up to 16 bits
static inline
uint16_t	getBits (uint8_t *d, int32_t offset, int16_t size) {
int16_t	i;
uint16_t	res	= 0;

	for (i = 0; i < size; i ++) {
	   res <<= 1;
	   res |= (d [offset + i]) & 01;
	}
	return res;
}

static inline
uint16_t	getBits_1 (uint8_t *d, int32_t offset) {
	return (d [offset] & 0x01);
}

static inline
uint16_t	getBits_2 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= (d [offset + 1] & 01);
	return res;
}

static inline
uint16_t	getBits_3 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	res	<<= 1;
	res	|= d [offset + 2];
	return res;
}

static inline
uint16_t	getBits_4 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	res	<<= 1;
	res	|= d [offset + 2];
	res	<<= 1;
	res	|= d [offset + 3];
	return res;
}

static inline
uint16_t	getBits_5 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	res	<<= 1;
	res	|= d [offset + 2];
	res	<<= 1;
	res	|= d [offset + 3];
	res	<<= 1;
	res	|= d [offset + 4];
	return res;
}

static inline
uint16_t	getBits_6 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	res	<<= 1;
	res	|= d [offset + 2];
	res	<<= 1;
	res	|= d [offset + 3];
	res	<<= 1;
	res	|= d [offset + 4];
	res	<<= 1;
	res	|= d [offset + 5];
	return res;
}

static inline
uint16_t	getBits_7 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	res	<<= 1;
	res	|= d [offset + 2];
	res	<<= 1;
	res	|= d [offset + 3];
	res	<<= 1;
	res	|= d [offset + 4];
	res	<<= 1;
	res	|= d [offset + 5];
	res	<<= 1;
	res	|= d [offset + 6];
	return res;
}

static inline
uint16_t	getBits_8 (uint8_t *d, int32_t offset) {
uint16_t	res	= d [offset];
	res	<<= 1;
	res	|= d [offset + 1];
	res	<<= 1;
	res	|= d [offset + 2];
	res	<<= 1;
	res	|= d [offset + 3];
	res	<<= 1;
	res	|= d [offset + 4];
	res	<<= 1;
	res	|= d [offset + 5];
	res	<<= 1;
	res	|= d [offset + 6];
	res	<<= 1;
	res	|= d [offset + 7];
	return res;
}


static inline
uint32_t	getLBits	(uint8_t *d,
	                         int32_t offset, int16_t amount) {
uint32_t	res	= 0;
int16_t		i;

	for (i = 0; i < amount; i ++) {
	   res <<= 1;
	   res |= (d [offset + i] & 01);
	}
	return res;
}

static inline
bool	check_CRC_bits (uint8_t *in, int32_t size) {
static
const uint8_t crcPolynome [] =
	{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};	// MSB .. LSB
int32_t	i, f;
uint8_t	b [16];
int16_t	Sum	= 0;

	memset (b, 1, 16);

	for (i = size - 16; i < size; i ++)
	   in [i] ^= 1;

	for (i = 0; i < size; i++) {
	   if ((b [0] ^ in [i]) == 1) {
	      for (f = 0; f < 15; f++) 
	         b [f] = crcPolynome [f] ^ b[f + 1];
	      b [15] = 1;
	   }
	   else {
	      memmove (&b [0], &b[1], sizeof (uint8_t ) * 15); // Shift
	      b [15] = 0;
	   }
	}

	for (i = 0; i < 16; i++)
	   Sum += b [i];

	return Sum == 0;
}

static inline
bool	check_crc_bytes (uint8_t *msg, int32_t len) {
int i, j;
uint16_t	accumulator	= 0xFFFF;
uint16_t	crc;
uint16_t	genpoly		= 0x1021;

	for (i = 0; i < len; i ++) {
	   int16_t data = msg [i] << 8;
	   for (j = 8; j > 0; j--) {
	      if ((data ^ accumulator) & 0x8000)
	         accumulator = ((accumulator << 1) ^ genpoly) & 0xFFFF;
	      else
	         accumulator = (accumulator << 1) & 0xFFFF;
	      data = (data << 1) & 0xFFFF;
	   }
	}
//
//	ok, now check with the crc that is contained
//	in the au
	crc	= ~((msg [len] << 8) | msg [len + 1]) & 0xFFFF;
	return (crc ^ accumulator) == 0;
}

