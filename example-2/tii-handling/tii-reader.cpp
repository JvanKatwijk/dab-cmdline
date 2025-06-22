#
/*
 *    Copyright (C) 2014 .. 2023
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of dab-cmdline
 *
 *    dab-cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dab-cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab-cmdline; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	<stdio.h>
#include	<string>
#include	<math.h>
#include	"dab-constants.h"
#include	"charsets.h"
#include	"tii-reader.h"

#define	SEPARATOR	';'
#define	COUNTRY		1
#define	CHANNEL		2
#define	ENSEMBLE	3
#define	EID		4
#define	TII		5
#define	LOCATION	6
#define	LATITUDE	7
#define	LONGITUDE	8
#define	ALTITUDE	9
#define	HEIGHT		10
#define	POLARIZATION	11
#define	FREQUENCY	12
#define	POWER		13
#define DIRECTION	14
#define	NR_COLUMNS	15

static
std::string Trim (const std::string& src) {
	return src;
	if (src.empty())
	   return "";

	size_t i = 0; // first index 
	size_t j = src. size () - 1; // last index of src

	while (i < j && isspace (src [i]))
	   i++;

	while (j > i && isspace (src [j]))
	   j--;

	return std::string (src, i, j - 2 );
}

		tiiReader::tiiReader	() {
	Rflag	= true;
}
		tiiReader::~tiiReader	() {
}
//
std::vector<cacheElement> tiiReader::readFile (const std::string &s) {
std::vector<cacheElement> res;
	if (s == "") {
	   return res;
	}

	res. resize (0);
	FILE	*f	= fopen (s. c_str (), "r+b");
	if (f == nullptr) 
	   return res;
	cacheElement ed;
	res. push_back (ed);	// the dummy one
	int	count = 1; 
	char	buffer [1024];
	std::vector<std::string> columnVector;
	int	shift	= 0;
	if (Rflag)
	   shift	= fgetc (f);
	int teller = 0;
	while (eread  (buffer, 1024, f, shift) != nullptr) {
	   cacheElement ed;
	   if (feof (f))
	      break;
	   teller ++;
	   columnVector. resize (0);
	   int columns = readColumns (columnVector, buffer, NR_COLUMNS);
	   if (columns < NR_COLUMNS)
	      continue;
	   ed. valid		= true;
	   ed. country		= Trim (columnVector [COUNTRY]);
	   ed. Eid		= get_Eid (columnVector [EID]);
	   ed. mainId		= get_mainId (columnVector [TII]);
	   ed. subId		= get_subId (columnVector [TII]);
	   ed. channel		= Trim (columnVector [CHANNEL]);
	   ed. ensemble 	= Trim (columnVector [ENSEMBLE]);
	   ed. transmitterName	= Trim (columnVector [LOCATION]);
	   ed. latitude		= convert (columnVector [LATITUDE]);
	   ed. longitude	= convert (columnVector [LONGITUDE]);
	   ed. power		= convert (columnVector [POWER]);
	   ed. altitude		= convert (columnVector [ALTITUDE]);
	   ed. height		= convert (columnVector [HEIGHT]);
	   ed. polarization	= Trim (columnVector [POLARIZATION]);
	   ed. frequency	= convert (columnVector[FREQUENCY]);
	   ed. direction	= Trim (columnVector [DIRECTION]);

	   if ((ed. mainId == 255) ||(ed. subId == 255))
	      ed. valid = false;
	   if ((ed. mainId == 0) || (ed. subId == 0))
	      ed. valid = false;
	   if (ed. ensemble == "")
	      ed. valid = false;
	   if (count >= (int) res. size ())
	      res. resize (res. size () + 100);
	   res. at (count) = ed;
	   count ++;
	}
	fclose (f);
	return res;
}
	
int	tiiReader::readColumns (std::vector<std::string> &v, char *b, int N) {
int charp	= 0;
char	tb [256];
int elementCount = 0;
std::string element;

	v. resize (0);
	while ((*b != 0) && (*b != '\n')) {
	   if (*b == SEPARATOR) {
	      tb [charp] = 0;
	      std::string  ss = toStringUsingCharset (tb, (CharacterSet)0x0F);
	      v. push_back (ss);
	      charp = 0;
	      elementCount ++;
	      if (elementCount >= N)
	         return N;
	   }
	   else
	      tb [charp ++] = *b;
	   b ++;
	}
	return elementCount;
}

//
char	*tiiReader::eread (char *buffer, int amount, FILE *f, uint8_t shift) {
char	*bufferP;

	if (fgets (buffer, amount, f) == nullptr)
	   return nullptr;
	bufferP	= buffer;
	if (Rflag) {
	   while (*bufferP != 0) {
	      if (shift != 0xAA)
	         *bufferP -= shift;
	      else
	         *bufferP ^= 0xAA;
	      bufferP ++;
	   }
	   *bufferP = 0;
	}
	return buffer;
}

float	tiiReader::convert (const std::string s) {
float	v;
	try {
	   v = std::stof (Trim (s));
	}
	catch (...) {
	   v = 0;
	}
	return v;
}

uint32_t tiiReader::get_Eid (const std::string s) {
uint32_t res;
	try {
	   res = std::stoi (s, 0, 16);
	}
	catch (...) {
	   res = 0;
	}
	return res;
}

uint8_t	tiiReader::get_mainId (const std::string s) {
uint16_t res;
	try {
	   res = std::stoi (s);
	}
	catch (...) {
	   res = 0;
	}
	return res / 100;
}

uint8_t tiiReader::get_subId (const std::string s) {
uint16_t res;
	try {
	   res = std::stoi (Trim (s));
	}
	catch (...) {
	   res = 0;
	}
	return res % 100;
}

