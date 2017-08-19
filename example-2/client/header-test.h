#
/*
 *    Copyright (C) 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the SDR-J
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
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
 *
 *
 *	Simple checker for te package header
 */
#ifndef	__HEADER_TEST__
#define	__HEADER_TEST__
#include	<stdint.h>
#include	<stdio.h>

class headerTest {
private:
	uint8_t b [8];
	int16_t	counter;
public:
		headerTest (void) {
	counter	= 0;
}
		~headerTest (void) {
}

void	shift (uint8_t d) {
	if ((counter == 0) && (d == 0xFF)) {
	   b [counter ++] = 0xFF;
	   return;
	}
	if ((counter == 1) && (d == 0x00)) {
	   b [counter ++] = 0x00;
	   return;
	}
	if ((counter == 2) && (d == 0xFF)) {
	   b [counter ++] = 0xFF;
	   return;
	}
	if ((counter == 3) && (d == 0x00)) {
	   b [counter ++] = 0x00;
	   return;
	}
	if (counter == 4) {
	   b [counter ++] = d;
	   return;
	}
	if (counter == 5) {
	   b [counter ++] = d;
	   return;
	}
	if ((counter == 6) && (d == 0x00)) {
	   b [counter ++] = d;
	   return;
	}
	if ((counter == 7) && ((d == 0x00) || (d = 0xFF))) {
	   b [counter ++] = d;
	fprintf (stderr, "header detected, %o %o %o (%d)\n",
	                  b [4], b [5], b [7], 
	                  (b [4] << 8) | (b [5] & 0xFF));
	   return;
	}
	counter = 0;
}

void	reset	(void) {
	counter = 0;
}

bool	hasHeader (void) {
	return counter == 8;
}

int16_t	length () {
	return (b [4] << 8) | b [5];
}
};

#endif

