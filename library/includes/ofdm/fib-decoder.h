#
/*
 *    Copyright (C) 2016 .. 2023
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of Qt-DAB
 *
 *    Qt-DAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Qt-DAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Qt-TAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#
#pragma once
//
#include	<cstdint>
#include	<cstdio>
#include	"msc-handler.h"
//
#include        <stdint.h>
#include        <stdio.h>
#include        <string>
#include        <mutex>
#include        <atomic>
#include        "dab-api.h"
#include        "dab-constants.h"

class	fibConfig;
class	ensemble;
class	fibDecoder {
public:
			fibDecoder		(API_struct *, void *);
			~fibDecoder		();

	void		clear_ensemble		();
	void		connect_channel		();
	void		disconnect_channel	();
	void		reset			();
	bool		syncReached		();

	uint16_t	get_announcing		(uint16_t);
	uint32_t	get_SId			(int);
	std::string	get_serviceName		(uint32_t /* SId */);
	std::string	get_ensembleName	();
	uint8_t		serviceType		(int);
	int		getServiceComp		(const std::string &);
	int		get_serviceComp		(uint32_t SId, int compnr);
	int		getServiceComp		(uint32_t, int);
	int		get_serviceComp_SCIds	(uint32_t, int);
	bool		isPrimary		(const std::string &);
	void		audioData		(int, audiodata &);
	void		packetData		(int, packetdata &);
	int		get_nrComps		(uint32_t);
	int		nrChannels		();
        uint8_t		get_ecc			();
        uint32_t	get_EId			();
	std::vector<int>	getFrequency		(const std::string &);
	int32_t		get_CIFcount		();	
	void		get_CIFcount		(int16_t &, int16_t &);
	uint32_t	julianDate		();
	int		freeSpace		();
	void		process_FIB		(uint8_t *, uint16_t);
private:
        API_struct      *theParameters;
        name_of_ensemble_t  name_of_ensemble; 
        serviceName_t   serviceName;
        void            *userData;

	fibConfig	*currentConfig;
	fibConfig	*nextConfig;
	ensemble	*theEnsemble;
	void		adjustTime		(int32_t *dateTime);

	void		process_FIG0		(uint8_t *);
	void		process_FIG1		(uint8_t *);
	void		FIG0Extension0		(uint8_t *);
	void		FIG0Extension1		(uint8_t *);
	void		FIG0Extension2		(uint8_t *);
	void		FIG0Extension3		(uint8_t *);
//	void		FIG0Extension4		(uint8_t *);
	void		FIG0Extension5		(uint8_t *);
//	void		FIG0Extension6		(uint8_t *);
	void		FIG0Extension7		(uint8_t *);
	void		FIG0Extension8		(uint8_t *);
	void		FIG0Extension9		(uint8_t *);
	void		FIG0Extension10		(uint8_t *);
//	void		FIG0Extension11		(uint8_t *);
//	void		FIG0Extension12		(uint8_t *);
	void		FIG0Extension13		(uint8_t *);
	void		FIG0Extension14		(uint8_t *);
//	void		FIG0Extension15		(uint8_t *);
//	void		FIG0Extension16		(uint8_t *);
	void		FIG0Extension17		(uint8_t *);
	void		FIG0Extension18		(uint8_t *);
	void		FIG0Extension19		(uint8_t *);
//	void		FIG0Extension20		(uint8_t *);
	void		FIG0Extension21		(uint8_t *);
//	void		FIG0Extension22		(uint8_t *);
//	void		FIG0Extension23		(uint8_t *);
//	void		FIG0Extension24		(uint8_t *);
//	void		FIG0Extension25		(uint8_t *);
//	void		FIG0Extension26		(uint8_t *);

	int16_t		HandleFIG0Extension1	(uint8_t *,
	                                         int16_t,
	                                         uint8_t, uint8_t, uint8_t);
	int16_t		HandleFIG0Extension2	(uint8_t *,
	                                         int16_t,
	                                         uint8_t, uint8_t, uint8_t);
	int16_t		HandleFIG0Extension3	(uint8_t *,
	                                         int16_t,
	                                         uint8_t, uint8_t, uint8_t);
	int16_t		HandleFIG0Extension5	(uint8_t *,
	                                         uint8_t, uint8_t, uint8_t,
	                                         int16_t);
	int16_t		HandleFIG0Extension8	(uint8_t *,
	                                         int16_t,
	                                         uint8_t, uint8_t, uint8_t);
	int16_t		HandleFIG0Extension13	(uint8_t *,
	                                         int16_t,
	                                         uint8_t, uint8_t, uint8_t);
	int16_t		HandleFIG0Extension21	(uint8_t*,
	                                         uint8_t, uint8_t, uint8_t,
	                                         int16_t);

	void		FIG1Extension0		(uint8_t *);
	void		FIG1Extension1		(uint8_t *);
//	void		FIG1Extension2		(uint8_t *);
//	void		FIG1Extension3		(uint8_t *);
	void		FIG1Extension4		(uint8_t *);
	void		FIG1Extension5		(uint8_t *);
	void		FIG1Extension6		(uint8_t *);

	mutex		fibLocker;
	std::atomic<int>	CIFcount;
	int16_t		CIFcount_hi;
	int16_t		CIFcount_lo;
	uint32_t	mjd;			// julianDate

	void		handle_announcement	(uint16_t SId,
	                                         uint16_t flags,
	                                         uint8_t SubChId);
};


