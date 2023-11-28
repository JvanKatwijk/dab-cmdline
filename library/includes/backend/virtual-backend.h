#
/*
 *    Copyright (C) 2014 .. 2017
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
 */
#
#pragma once

#include	<stdint.h>
#include	<stdio.h>

#define	CUSize	(4 * 16)

class	virtualBackend {
public:
		virtualBackend	(int16_t, int16_t);
virtual		~virtualBackend	();
virtual int32_t	process		(int16_t *, int16_t);
virtual void	stopRunning	();
virtual	void	stop		();
	int16_t	startAddr	();
	int16_t	Length		();
protected:
	int16_t startAddress;
	int16_t	segmentLength;
};

