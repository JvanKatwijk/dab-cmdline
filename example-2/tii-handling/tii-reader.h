#
/*
 *    Copyright (C) 2014 .. 2023
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of dab-cmdline
 *
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#pragma once

#include	<stdio.h>
#include	<string>
#include	<math.h>
#include	<vector>
#include	"dab-constants.h"
#include	"cacheElement.h"

#ifdef	__MING32__
#include	<winsock2.h>
#include	<windows.h>
#endif
class	tiiReader {
public:
		tiiReader	();
		~tiiReader	();
//
	std::vector<cacheElement>
	        readFile	(const std::string &s);
	int	readColumns	(std::vector<std::string> &v, char *b, int N);
	char	*eread		(char *buffer, int amount,
	                                        FILE *f, uint8_t shift);
private:
	float	convert		(const std::string);
	uint32_t get_Eid	(const std::string);
	uint8_t	get_mainId	(const std::string);
	uint8_t	get_subId	(const std::string);
	bool	Rflag;
};

