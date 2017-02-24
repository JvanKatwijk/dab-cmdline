#
/*
 *    Copyright (C)  2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the DAB-cmdline program.
 *    Many of the ideas as implemented in DAB-cmdline are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    DAB-cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB-cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB-cmdline; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __DAB_CLASS__
#define __DAB_CLASS__

#include	"dab-constants.h"
#include	"dab-api.h"
#include	<vector>
#include	<string>
#include	<list>
#include	<atomic>
#include	<thread>

#include	"ofdm-processor.h"
#include	"fic-handler.h"
#include	"msc-handler.h"
#include	"ringbuffer.h"
#include	"ensemble-handler.h"

#include	<mutex>
#include	<condition_variable>

class	virtualInput;

class dabClass {
public:
        		dabClass	(virtualInput	*,
	                         	DabParams	*,
	                         	dabBand,
	                                int16_t,		// waiting time
	                         	cb_audio_t,
	                                cb_data_t);
			~dabClass	(void);
	void		dab_gain	(uint16_t);
	bool		dab_running	(void);
	bool		dab_channel	(std::string s);
	void		dab_run		(cb_ensemble_t);
	void		dab_stop	(void);
	bool		dab_service	(std::string, cb_programdata_t);
	bool		dab_service	(int32_t, cb_programdata_t);
	bool		ensembleArrived	(void);
private:
	virtualInput	*inputDevice;
	DabParams	*dabModeParameters;
	dabBand		theBand;
	int16_t		waitingTime;
	cb_audio_t	soundOut;
	cb_data_t	dataOut;
	std::thread     threadHandle;
	void		run_dab		(cb_ensemble_t);
	std::atomic<bool> run;
	ofdmProcessor	*my_ofdmProcessor;
	ficHandler	*my_ficHandler;
	mscHandler	*my_mscHandler;
	int16_t		deviceGain;
//

	std::string	requestedProgram;
	std::string	requestedChannel;
	ensembleHandler theEnsemble;
};

#endif
