#
/*
 *
 *    Copyright (C) 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB
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
#include	"tii_table.h"


	tii_element::tii_element (int16_t subId, int16_t TD,
	                     float latitudeOffset, float longitudeOffset) {
	this	-> subId	= subId;
	this	-> TD		= TD;
	this	-> latitudeOffset = latitudeOffset;
	this	-> longitudeOffset = longitudeOffset;
}

	tii_table::tii_table (void) {
}

	tii_table::~tii_table (void) {
}

void	tii_table::cleanUp (void) {
	offsets. clear ();
	mainId		= -1;
}

void	tii_table::add_main	(int16_t mainId, float latitude, float longitude) {
	if (this -> mainId > 0)
	   return;
	this	-> mainId = mainId;
	this	-> latitude	= latitude;
	this	-> longitude	= longitude;
}

void	tii_table::add_element (tii_element *t) {
uint16_t i;

	for (i = 0; i < offsets. size (); i ++)
	   if (offsets [i]. subId == t -> subId)
	      return;

	offsets. push_back (*t);
}

std::complex<float> tii_table::get_coordinates (int16_t mainId,
	                               int16_t subId, bool *success) {
uint16_t i;
float x, y;

	*success	= false;
	if (this -> mainId != mainId)
	   return std::complex<float> (0, 0);

	for (i = 0; i < offsets. size (); i ++) {
	   if (offsets [i]. subId != subId)
	      continue;

	   x	= latitude + offsets [i]. latitudeOffset;
	   y	= longitude + offsets [i]. longitudeOffset;
	
	   *success = true;
	   return std::complex<float> (x, y);
	}
	return std::complex<float> (0, 0);
}

//
//	Function extension contributed by Hayati Ayguen
// mainId < 0 (-1) => don't check mainId
// subId == -1 => deliver first available offset
// subId == -2 => deliver coarse coordinates
std::complex<float>
	tii_table::get_coordinates (int16_t mainId, int16_t subId,
	                            bool *success,
	                            int16_t *pMainId, int16_t *pSubId,
	                            int16_t *pTD) {
uint16_t i;
float x, y;

	*success	= false;

	if ( (this -> mainId != mainId && mainId >= 0) || this->mainId < 0 )
	   return std::complex<float> (0, 0);

	// subId == -2 => deliver coarse coordinates
	if (subId == -2) {
	   *success = true;
	   if (pMainId)		*pMainId	= this -> mainId;
	   if (pSubId)		*pSubId		= 99;	/* invalid value */
	   if (pTD)		*pTD		= -1;	/* invalid value */
	   return std::complex<float> (latitude, longitude);
	}

	for (i = 0; i < offsets. size (); i ++) {
	   // subId == -1 => deliver first available offset
	   if (offsets [i]. subId != subId && subId != -1)
	      continue;

	   x	= latitude + offsets [i]. latitudeOffset;
	   y	= longitude + offsets [i]. longitudeOffset;

	   if (pMainId != nullptr)	*pMainId	= this -> mainId;
	   if (pSubId != nullptr)	*pSubId		= offsets [i]. subId;
	   if (pTD != nullptr)		*pTD		= offsets [i]. TD;
	   *success = true;
	   return std::complex<float> (x, y);
	}
	return std::complex<float> (0, 0);
}

void	tii_table::print_coordinates	(void) {
uint16_t	i;
	if (mainId < 0)
	   return;

	fprintf (stderr, "Transmitter coordinates (%f %f)\n",
	                              latitude, longitude);
	for (i = 0; i < offsets. size (); i ++) {
	   float x	= latitude + offsets [i]. latitudeOffset;
	   float y	= longitude + offsets [i]. longitudeOffset;
	   fprintf (stderr, "%d\t-> %f\t%f\n", offsets [i]. subId, x, y);
	}
}

