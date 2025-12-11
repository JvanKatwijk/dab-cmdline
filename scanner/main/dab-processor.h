#
/*
 *    Copyright (C) 2025
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the 2-nd DAB scannner and borrowed
 *    for the Qt-DAB repository of the same author
 *
 *    DAB scannner is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB scanner is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB scanner; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#pragma once
/*
 *
 */
#include	"dab-constants.h"
#include	"dab-api.h"
#include	<thread>
#include	<atomic>
#include	<stdint.h>
#include	<vector>
#include	"dab-params.h"
#include	"correlator.h"
#include	"freqsyncer.h"
#include	"ofdm-decoder.h"
#include	"fic-handler.h"
#include	"ringbuffer.h"
#include	"sample-reader.h"
#include	"tii-detector.h"
#include	"phasetable.h"
//
class	deviceHandler;

class dabProcessor {
public:
		dabProcessor  	(deviceHandler *,
	                         API_struct	*,
	                         void	*);
	virtual ~dabProcessor	();
	void	reset		();
	void	stop		();
	void	setOffset	(int);
	void	start		();
	bool	signalSeemsGood	();
	void	show_Corrector	(int);
//      inheriting from our delegates
	bool		syncReached		();
	uint8_t		serviceType		(const std::string &);
	void		dataforAudioService	(const std::string &,
	                                                       audiodata &);
	void		dataforAudioService	(const std::string &,
	                                             audiodata &, int16_t);
	void		dataforDataService	(const std::string &,
	                                                packetdata &);
	void		dataforDataService	(const std::string &,
	                                             packetdata &, int16_t);
	int32_t		get_SId			(const std::string &s);
	std::string	get_serviceName		(int32_t);
	void		set_audioChannel        (audiodata &);
	void		set_dataChannel         (packetdata &);
	void		reset_msc		();
	std::string	get_ensembleName	();
	void		clearEnsemble		();

	int		get_nrComponents	();
	contentType	content			(int);
	int		get_snr			();
private:
	deviceHandler	*inputDevice;
	phaseTable	theTable;
	dabParams	params;
	sampleReader	myReader;
	correlator	myCorrelator;
	freqSyncer	myFreqSyncer;
	TII_Detector	my_TII_Detector;
	ofdmDecoder	my_ofdmDecoder;
	ficHandler	my_ficHandler;
	syncsignal_t	syncsignalHandler;
	tii_data_t	show_tii;
	void		call_systemData (bool, int16_t, int32_t);
	std::thread	threadHandle;
	void		*userData;
	std::atomic<bool>	running;
	bool		isSynced;
	int		threshold;
	int		snr;
	int32_t		T_null;
	int32_t		T_u;
	int32_t		T_s;
	int32_t		T_g;
	int32_t		T_F;
	int32_t		nrBlocks;
	int32_t		carriers;
	int32_t		carrierDiff;
	bool		wasSecond	(int16_t, dabParams *);
	int		tii_counter;
virtual	void		run		();
};

