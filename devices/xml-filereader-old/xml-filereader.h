#
/*
 *    Copyright (C) 2013 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library
 *
 *    DAB library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef	__XML_FILEREADER__
#define	__XML_FILEREADER__

#include	<string>
#include	<atomic>
#include	<thread>
#include	"dab-constants.h"
#include	"device-handler.h"
#include	"ringbuffer.h"

class	xmlDescriptor;
class	xml_Reader;
/*
 */
class	xml_fileReader: public deviceHandler {
public:
				xml_fileReader	(std::string, bool);
                		~xml_fileReader	();
	int32_t			getSamples	(std::complex<float> *,
	                                                         int32_t);
	int32_t			Samples		();
	bool			restartReader	(int32_t);
	void			stopReader	(void);
private:
	bool			continue_on_eof;
	FILE			*theFile;
	std::atomic<bool>	running;
	std::string		fileName;
	RingBuffer<std::complex<float>>	*_I_Buffer;
	uint32_t		filePointer;
	xmlDescriptor		*theDescriptor;
	xml_Reader		*theReader;
};

#endif

