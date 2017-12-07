#
/*
 *    Copyright (C) 2015, 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB-library
 *
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

#ifndef	__RADIODATA__
#define	__RADIODATA__

typedef struct  {
	uint8_t         theMode;
	std::string     theChannel;
	std::string	serviceName;
	uint8_t         theBand;
	int16_t         ppmCorrection;
	int             theGain;
	std::string     soundChannel;
	int16_t         latency;
	int16_t         waitingTime;
	bool            autogain;
	std::string	hostname;
	int		basePort;
	int		serverPort;
} radioData;

#endif

