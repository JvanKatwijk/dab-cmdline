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

#pragma once
#include	<stdint.h>
#include	<vector>
#include	<string>

//	ensemble information relates to FIG1, basically some
//	general, i.e. ensemble wide, data and mapping tables for
//	primary and secondary services to SId's resp (SId x SCIds)
//	Itis now completely separated from the FIG1 tables
class ensemble {
public:
		ensemble	() {
	   reset ();
	}
		~ensemble	() {
	}

	uint8_t		eccByte;
	uint8_t		lto;
	uint16_t	EId;
	std::string	ensembleName;
	bool		namePresent;
	typedef struct {
	   std::string	name;
	   std::string	shortName;
	   uint32_t	SId;
	   uint8_t	programType;
	   uint8_t	SCIds;
	   std::vector<int>	fmFrequencies;
	} service;

	bool	isSynced;
	std::vector<service> primaries;
	std::vector<service> secondaries;

	void	reset		();
	uint32_t serviceToSId	(const std::string &s);
	std::string	SIdToserv	(uint32_t SId);
	int	programType	(uint32_t);
	std::vector<int>	fmFrequencies	(uint32_t);
};


