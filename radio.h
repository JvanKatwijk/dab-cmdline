#
/*
 *    Copyright (C)  2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _GUI
#define _GUI

#include	"dab-constants.h"

#include	<vector>
#include	<string>
#include	<list>

#include	"ofdm-processor.h"
#include	"fic-handler.h"
#include	"msc-handler.h"
#include	"ringbuffer.h"
#include	"ensemble-handler.h"

#include	<mutex>
#include	<condition_variable>

class	virtualInput;
class	audioSink;

class Radio {
public:
        	Radio		(std::string,
	                         uint8_t,
	                         std::string,
	                         std::string,	// channel
	                         std::string,	// program
	                         int,		// gain (0 .. 100)
	                         std::string);	// audiochannel
		~Radio		(void);

private:
	int16_t		threshold;
	void		setModeParameters	(uint8_t);

	DabParams	dabModeParameters;
	uint8_t		dabBand;
	virtualInput	*inputDevice;
	ofdmProcessor	*my_ofdmProcessor;
	ficHandler	*my_ficHandler;
	mscHandler	*my_mscHandler;
	audioSink	*soundOut;
const	char		*get_programm_type_string (uint8_t);
const	char		*get_programm_language_string (uint8_t);

	std::string		requestedProgram;
	std::string		requestedChannel;
	ensembleHandler theEnsemble;
	int		coarseCorrector;
	int		fineCorrector;
	bool		setDevice		(std::string);
	void		TerminateProcess	(void);
	bool		setChannel		(std::string);
	bool		setService		(std::string);

	void		set_fineCorrectorDisplay	(int);
	void		set_coarseCorrectorDisplay	(int);
//
//
	std::mutex	g_lockqueue;
	std::mutex	labelMutex;
	std::condition_variable g_queuecheck;
	std::list<std::string> labelQueue;
//
//	Kind of signal handler here
public:
	void		showLabel		(std::string);
};

#endif
