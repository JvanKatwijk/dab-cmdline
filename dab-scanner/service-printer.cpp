#
/*
 *    Copyright (C) 2013, 2014, 2015, 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the Qt-DAB (formerly SDR-J, JSDR).
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
 *    along with Qt-DAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	<stdio.h>
#include	"service-printer.h"
#include	"dab_tables.h"
#include	"dab-api.h"


void	print_ensembleData (FILE *f,
	                    void *theRadio,
	                    std::string currentChannel,
	                    std::string ensembleLabel,
	                    uint32_t	ensembleId) {

	if (ensembleLabel == std::string (""))
	   return;

	fprintf (f, "\n\nEnsemble %s; ensembleId %X; channel %s; \n\n",
	            ensembleLabel. c_str (),
	            ensembleId,
	            currentChannel. c_str ());
}

void	print_audioheader (FILE *f) {
	fprintf (f, "\nAudio services\nprogram name;country;serviceId;subchannelId;start address;length (CU); bit rate;DAB/DAB+; prot level; code rate; language; program type\n\n");
}

void	print_audioService (FILE *f,
	                    void *theRadio,
	                    std::string serviceName,
	                    audiodata *d) {
bool	success;

	if (!d -> defined)
	   return;

	std::string protL	= getProtectionLevel (d -> shortForm,
	                                              d -> protLevel);
	std::string codeRate	= getCodeRate (d -> shortForm, d -> protLevel);
	uint8_t ecc_byte	= dab_getExtendedCountryCode (theRadio, &success);
	uint32_t serviceId	= dab_getSId (theRadio, serviceName. c_str ());
	uint8_t countryId = (serviceId >> 12) & 0xF;
	const char *country	= getCountry (ecc_byte, countryId);
	if (country == nullptr)
	   country = " ";
	uint8_t ITabId	=  dab_getInternationalTabId (theRadio, &success);
	std::string programType	= getProgramType (success,
	                                          ITabId, d -> programType);
	                          
	fprintf (f, "%s;%s;%X;%d;%d;%d;%d;%s;%s;%s;%s;%s;\n",
	             serviceName. c_str (),
	             country,
	             serviceId,
	             d -> subchId,
	             d -> startAddr,
	             d -> length,
	             d -> bitRate,
	             getASCTy  (d -> ASCTy),
	             protL. c_str (),
	             codeRate. c_str (),
	             getLanguage (d -> language),
	             programType. c_str () );
}

void	print_dataHeader (FILE *f) {
	fprintf (f, "\n\n\nData Services\nprogram name;;serviceId;subchannelId;start address;length (CU); bit rate; FEC; prot level; appType ; subService ; \n\n");
}

void	print_dataService (FILE		*f,
	                   void		*theRadio,
	                   std::string	serviceName,
	                   uint8_t	compnr,
	                   packetdata	*d) {
bool	success;

	if (!d -> defined)
	   return;
	   
	std::string protL	= getProtectionLevel (d -> shortForm,
	                                              d -> protLevel);
	std::string codeRate	= getCodeRate (d -> shortForm,
	                                       d -> protLevel);
	uint8_t ecc_byte	= dab_getExtendedCountryCode (theRadio, &success);
	uint32_t serviceId	= dab_getSId (theRadio, serviceName. c_str ());
	uint8_t countryId = (serviceId >> 12) & 0xF;
	const char *country	= getCountry (ecc_byte, countryId);
	if (country == nullptr)
	   country = "   ";
	fprintf (f, "%s;%s;%X;%d;%d;%d;%d;%s;%s;%s;%s;;\n",
	             serviceName. c_str (),
	             country,
	             serviceId,
	             d -> subchId,
	             d -> startAddr,
	             d -> length,
	             d -> bitRate,
	             getFECscheme (d -> FEC_scheme),
	             protL. c_str (),
	             getUserApplicationType (d -> appType),
	             compnr == 0 ? "no": "yes");
}

