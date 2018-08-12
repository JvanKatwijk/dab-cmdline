#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library
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
#ifndef	__RAW_FILES__
#define	__RAW_FILES__

#include        "ringbuffer.h"
#include        "device-handler.h"
#include        <thread>
#include        <atomic>

typedef	void (*device_eof_callback_t)(void * userData);
/*
 */
class	rawFiles: public deviceHandler {
public:
			rawFiles	(std::string, bool);
			rawFiles	(std::string,
	                                 double fileOffset,
	                                 device_eof_callback_t eofHandler,
	                                 void * userData);
	       		~rawFiles	(void);
	int32_t		getSamples	(std::complex<float> *, int32_t);
	uint8_t		myIdentity	(void);
	int32_t		Samples		(void);
	bool		restartReader	(void);
	void		stopReader	(void);
private:
	std::string	fileName;
	double		fileOffset;
	device_eof_callback_t	eofHandler;
	void		*userData;
virtual	void		run		(void);
	RingBuffer<std::complex<float>>	*_I_Buffer;
	int32_t		readBuffer	(std::complex<float> *, int32_t);

	std::thread	workerHandle;
	int32_t		bufferSize;
	FILE		*filePointer;
	std::atomic<bool> running;
	int64_t		currPos;
};

#endif

