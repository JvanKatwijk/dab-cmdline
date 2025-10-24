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

#include "band-handler.h"

dabFrequency bandIII_frequencies [] = {
{"5A",    174928},
{"5B",    176640},
{"5C",    178352},
{"5D",    180064},
{"6A",    181936},
{"6B",    183648},
{"6C",    185360},
{"6D",    187072},
{"7A",    188928},
{"7B",    190640},
{"7C",    192352},
{"7D",    194064},
{"8A",    195936},
{"8B",    197648},
{"8C",    199360},
{"8D",    201072},
{"9A",    202928},
{"9B",    204640},
{"9C",    206352},
{"9D",    208064},
{"10A",   209936},
{"10B",   211648},
{"10C",   213360},
{"10D",   215072},
{"11A",   216928},
{"11B",   218640},
{"11C",   220352},
{"11D",   222064},
{"12A",   223936},
{"12B",   225648},
{"12C",   227360},
{"12D",   229072},
{"13A",   230748},
{"13B",   232496},
{"13C",   234208},
{"13D",   235776},
{"13E",   237488},
{"13F",   239200},
{nullptr, 0}
};


bandHandler::bandHandler	() {
	for (int i = 0; bandIII_frequencies [i]. fKHz != 0; i ++)
	   theFreqs. push_back (bandIII_frequencies [i]);
}

bandHandler::~bandHandler	() {}

