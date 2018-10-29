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

//
//	talking to the dab-xxx-6 program is through a port (default BASE_PORT)

#ifndef	__PROTOCOL__
#define	__PROTOCOL__

//	The radio will react on commands
//	The commands consist of a package with two components
//	the command identifier and a 0-terminated asci string
//
//	e.g. Q_GAIN "70" or Q_CHANNEL "11C"
#define Q_QUIT		0100
#define Q_IF_GAIN_REDUCTION	0101
#define Q_AUDIO_GAIN	0102
#define Q_LNA_STATE	0103
#define	Q_AUTOGAIN	0104
#define Q_CHANNEL       0105
#define Q_SERVICE       0106
#define	Q_RESET		0107

//	The radio will send some data back through port + 1
//	in all cases the format of the message is
//	a one byte identifier followed by a 0-terminated string

#define Q_ENSEMBLE      0100
#define Q_SERVICE_NAME  0101
#define Q_TEXT_MESSAGE  0102
#define	Q_PROGRAM_DATA	0103

//
//	packetstructure
//	byte 0	0xfF
//	byte 1	length  of the payload (including the header,
//	                                so the shortest one is 3)
//	byte 2	key (see above)
//	byte 3 .. 3 + length payload
//	byte 3 + length .. PACKET_SIZE - 1 usused
#endif

