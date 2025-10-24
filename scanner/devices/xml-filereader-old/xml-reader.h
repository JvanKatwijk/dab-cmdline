#
/*
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the eti-cmdline program
 *
 *    eti-cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    eti-cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with eti-cmdline; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#pragma once

#include	<thread>
#include	<stdio.h>
#include	"ringbuffer.h"
#include	<stdint.h>
#include	<complex>
#include	<vector>
#include	<atomic>

class	xml_fileReader;
class	xmlDescriptor;

class	xml_Reader {
public:
			xml_Reader (FILE		*f,
	                            xmlDescriptor	*fd,
	                            uint32_t		filePointer,
	                            RingBuffer<std::complex<float>> *b,
	                            bool		continue_on_eof);
			~xml_Reader	();
	void		stopReader	();
private:
	bool		continue_on_eof;
	FILE		*file;
	xmlDescriptor	*fd;
	uint32_t	filePointer;
	RingBuffer<std::complex<float>> *sampleBuffer;
	int		nrElements;
	int		samplesToRead;
	std::atomic<bool> running;
	void		run ();
	std::thread	threadHandle;
	int		compute_nrSamples 	(FILE *f, int blockNumber);
	int		readSamples		(FILE *f, 
	                                       void(xml_Reader::*)(FILE *,
	                                          std::complex<float> *, int));
	void		readElements_IQ		(FILE *f,
	                                         std::complex<float> *, int amount);
	void		readElements_QI		(FILE *f, 
	                                         std::complex<float> *, int amount);
	void		readElements_I		(FILE *f, 
	                                         std::complex<float> *, int amount);
	void		readElements_Q		(FILE *f, 
	                                         std::complex<float> *, int amount);
//
//	for the conversion - if any
	int16_t         convBufferSize;
        int16_t         convIndex;
        std::vector <std::complex<float> >   convBuffer;
        int16_t         mapTable_int   [2048];
        float           mapTable_float [2048];
};

