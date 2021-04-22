#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB-library program
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
#
#ifndef	__DAB_PROCESSOR__
#define	__DAB_PROCESSOR__
/*
 *
 */
#include	"dab-constants.h"
#include	<thread>
#include	<atomic>
#include	<stdint.h>
#include	<vector>
#include	"dab-params.h"
#include	"phasereference.h"
#include	"ofdm-decoder.h"
#include	"fic-handler.h"
#include	"msc-handler.h"
#include	"ringbuffer.h"
#include	"dab-api.h"
#include	"sample-reader.h"
#include	"tii-detector.h"
//
class	deviceHandler;

class dabProcessor {
public:
		dabProcessor  	(deviceHandler *,
	                         API_struct	*,
	                         RingBuffer<std::complex<float>> *,
                                 RingBuffer<std::complex<float>> *,
	                         void	*);
	virtual ~dabProcessor	();
	void	reset		();
	void	stop		();
	void	setOffset	(int);
	void	start		();
	bool	signalSeemsGood	();
	void	show_Corrector	(int);
//      inheriting from our delegates
	void		setSelectedService	(std::string);
	uint8_t		kindofService           (std::string);
	void		dataforAudioService     (std::string,   audiodata *);
	void		dataforAudioService     (std::string,
	                                             audiodata *, int16_t);
	void		dataforDataService      (std::string,   packetdata *);
	void		dataforDataService      (std::string,
	                                             packetdata *, int16_t);
	int32_t		get_SId			(std::string s);
	std::string	get_serviceName		(int32_t);
	void		set_audioChannel        (audiodata *);
	void		set_dataChannel         (packetdata *);
	std::string	get_ensembleName        (void);
	void		clearEnsemble           (void);
	void		reset_msc		(void);
private:
	deviceHandler	*inputDevice;
	dabParams	params;
	sampleReader	myReader;
	phaseReference	phaseSynchronizer;
	TII_Detector	my_TII_Detector;
	ofdmDecoder	my_ofdmDecoder;
	ficHandler	my_ficHandler;
	mscHandler	my_mscHandler;
	syncsignal_t	syncsignalHandler;
	systemdata_t	systemdataHandler;
	programdata_t	programdataHandler;
	tii_data_t	show_tii;
	void		call_systemData (bool, int16_t, int32_t);
	std::thread	threadHandle;
	void		*userData;
	std::atomic<bool>	running;
	bool		isSynced;
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
virtual	void		run		(void);
};
#endif

