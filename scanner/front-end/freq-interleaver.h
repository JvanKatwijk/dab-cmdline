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
#
#pragma once

#include	<stdint.h>
#include	"dab-constants.h"
#include	"dab-params.h"

/**
  *	\class interLeaver
  *	Implements frequency interleaving according to section 14.6
  *	of the DAB standard
  */
class	interLeaver {
public:
		interLeaver	();
		~interLeaver	();
	int16_t	mapIn		(int16_t);
private:
	dabParams	params;
	void	createMapper	(int16_t, int16_t,
                                 int16_t, int16_t, int16_t *);
	int16_t	*permTable;
};

