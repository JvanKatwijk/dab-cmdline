#
/*
 *    Copyright (C)  2009, 2010, 2011
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the main program for the DAB library
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
 *
 */

#ifndef __FILE_SINK__
#define	__FILE_SINK__
#include	<stdio.h>
#include	"audio-base.h"
#include	"ringbuffer.h"
#include	<string>

class	fileSink  : public audioBase {
public:
	                fileSink		(std::string, bool *);
			~fileSink		(void);
	void		stop			(void);
	void		restart			(void);
private:
	void		audioOutput		(float *, int32_t);
	FILE		*outputFile;
	bool		audioOK;
};

#endif

