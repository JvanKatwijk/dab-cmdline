#
/*
 *    Copyright (C) 2013 .. 2017
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
#include	"xml-filereader.h"
#include	<cstdio>
#include	<unistd.h>
#include	<cstdlib>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<ctime>
#include	"device-exceptions.h"
#include	"xml-descriptor.h"
#include	"xml-reader.h"

#define	INPUT_FRAMEBUFFERSIZE	8 * 32768
//
//
	xml_fileReader::xml_fileReader (std::string fileName,
	                                bool	continue_on_eof) {
	this -> fileName	= fileName;
	_I_Buffer	= new RingBuffer<std::complex<float>>(INPUT_FRAMEBUFFERSIZE);
	theFile	= fopen (fileName.c_str (), "rb");
	if (theFile == nullptr) {
	   DEBUG_PRINT ("file %s cannot open\n",
	                                   fileName. c_str ());
	   delete _I_Buffer;
	   throw OpeningFileFailed(fileName.c_str(),strerror(errno));
	}

	bool	ok	= false;
	theDescriptor	= new xmlDescriptor (theFile, &ok);
	if (!ok) {
	   DEBUG_PRINT ("%s probably not an xml file\n",
	                               fileName. c_str ());
	   delete _I_Buffer;
	   throw OpeningFileFailed(fileName.c_str(),"Not a xml file");
	}

	DEBUG_PRINT ("nrElements = %d\n",
	             theDescriptor -> blockList [0].nrElements);
	theReader	= nullptr;
}

	xml_fileReader::~xml_fileReader	() {
	if (theReader != nullptr)
	   delete theReader;

	if (theFile != nullptr)
	   fclose (theFile);

	delete _I_Buffer;
	delete	theDescriptor;
}

bool	xml_fileReader::restartReader (int32_t freq) {
	(void)freq;
	if (theReader != nullptr)
	   return true;
	theReader	= new xml_Reader (theFile,
	                                  theDescriptor,
	                                  5000,
	                                  _I_Buffer,
	                                  continue_on_eof);
	return true;
}

void	xml_fileReader::stopReader () {
	if (theReader != nullptr)
	   delete theReader;
	theReader = nullptr;
}

//	size is in "samples"
int32_t	xml_fileReader::getSamples	(std::complex<float> *V,
	                                 int32_t size) {

	if (theFile == nullptr)		// should not happen
	   return 0;

	while ((int32_t)(_I_Buffer -> GetRingBufferReadAvailable()) < size)
	   usleep (1000);

	return _I_Buffer	-> getDataFromBuffer (V, size);
}

int32_t	xml_fileReader::Samples	() {
	if (theFile == nullptr)
	   return 0;
	return _I_Buffer -> GetRingBufferReadAvailable();
}
