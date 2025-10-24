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
#
#pragma once
//
#include	<cstdint>
#include	<cstdio>
#include	<mutex>
#include	<atomic>

#include	"ensemble.h"
class	fibConfig;

#include	"dab-api.h"

class	fibDecoder {
public:
			fibDecoder		(API_struct *, void *);
			~fibDecoder		();

	void		clearEnsemble		();
	void		connectChannel		();
	void		disconnectChannel	();
	bool		syncReached		();
	void		reset			();
//
//	The real interface 
	uint32_t	getSId			(int);
	uint8_t		serviceType		(int);
	int		getNrComps		(const uint32_t);
//
//	not well chosen name, "subChannels" are meant
	int		nrChannels		();
	int		getServiceComp		(const std::string &);
	int		getServiceComp		(const uint32_t, const int);
	int		getServiceComp_SCIds	(const uint32_t, const int);
	bool		isPrimary		(const std::string &);
	void		audioData		(const int, audiodata &);
	void		packetData		(const int, packetdata &);
	uint16_t	getAnnouncing		(uint16_t);
	std::vector<int>	getFrequency	(const std::string &);
	bool		evenFrame		();	
	void		getCIFcount		(int16_t &, int16_t &);
	uint32_t	julianDate		();
	int		freeSpace		();
	int		get_nrComponents	();
	contentType	content			(int);

	uint8_t		get_ecc			();
	uint16_t	get_EId			();
	std::string	get_ensembleName	();
protected:
	void		process_FIB		(uint8_t *, uint16_t);
private:
	ensemble	theEnsemble;
	fibConfig	*currentConfig;
	fibConfig	*nextConfig;
	void		adjustTime		(int32_t *dateTime);

	name_of_ensemble_t  name_of_ensemble;
        serviceName_t   serviceName;
	theTime_t	timeHandler;
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
	void		FIG0Extension15		(uint8_t *);
//	void		FIG0Extension16		(uint8_t *);
	void		FIG0Extension17		(uint8_t *);
	void		FIG0Extension18		(uint8_t *);
	void		FIG0Extension19		(uint8_t *);
	void		FIG0Extension20		(uint8_t *);
	void		FIG0Extension21		(uint8_t *);
//	void		FIG0Extension22		(uint8_t *);
//	void		FIG0Extension23		(uint8_t *);
//	void		FIG0Extension24		(uint8_t *);
//	void		FIG0Extension25		(uint8_t *);
//	void		FIG0Extension26		(uint8_t *);

	int16_t		HandleFIG0Extension1	(uint8_t *,
	                                         int16_t,
	                                         const uint8_t,
	                                         const uint8_t,
	                                         const uint8_t);
	int16_t		HandleFIG0Extension2	(uint8_t *,
	                                         int16_t,
	                                         const uint8_t,
	                                         const uint8_t,
	                                         const uint8_t);
	int16_t		HandleFIG0Extension3	(uint8_t *,
	                                         int16_t,
	                                         const uint8_t,
	                                         const uint8_t,
	                                         const uint8_t);
	int16_t		HandleFIG0Extension5	(uint8_t *,
	                                         uint16_t,
	                                         const uint8_t,
	                                         const uint8_t,
	                                         const uint8_t);
	int16_t		HandleFIG0Extension8	(uint8_t *,
	                                         int16_t,
	                                         const uint8_t,
	                                         const uint8_t,
	                                         const uint8_t);
	int16_t		HandleFIG0Extension13	(uint8_t *,
	                                         int16_t,
	                                         const uint8_t,
	                                         const uint8_t,
	                                         const uint8_t);
	int16_t		HandleFIG0Extension20	(uint8_t *,
	                                         uint16_t,
	                                         const uint8_t,
	                                         const uint8_t,
	                                         const uint8_t);
	int16_t		HandleFIG0Extension21	(uint8_t*,
	                                         uint16_t,
	                                         const uint8_t,
	                                         const uint8_t,
	                                         const uint8_t);

	void		FIG1Extension0		(uint8_t *);
	void		FIG1Extension1		(uint8_t *);
//	void		FIG1Extension2		(uint8_t *);
//	void		FIG1Extension3		(uint8_t *);
	void		FIG1Extension4		(uint8_t *);
	void		FIG1Extension5		(uint8_t *);
	void		FIG1Extension6		(uint8_t *);

	std::mutex		fibLocker;
	std::atomic<int>	CIFcount;
	std::atomic<int16_t>	CIFcount_hi;
	std::atomic<int16_t>	CIFcount_lo;
	uint32_t	mjd;			// julianDate

	void		handleAnnouncement	(uint16_t SId,
	                                         uint16_t flags,
	                                         uint8_t SubChId);
};


