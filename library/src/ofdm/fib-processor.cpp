#
/*
 *    Copyright (C) 2014 - 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB-library
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
 *
 * 	fib and fig processor
 */
#include	"fib-processor.h"
#include	<cstring>
#include	"charsets.h"
#include	"ensemble-handler.h"
//
//
// Tabelle ETSI EN 300 401 Page 50
// Table is copied from the work of Michael Hoehn
   const int ProtLevel[64][3]   = {{16,5,32},	// Index 0
                                   {21,4,32},
                                   {24,3,32},
                                   {29,2,32},
                                   {35,1,32},	// Index 4
                                   {24,5,48},
                                   {29,4,48},
                                   {35,3,48},
                                   {42,2,48},
                                   {52,1,48},	// Index 9
                                   {29,5,56},
                                   {35,4,56},
                                   {42,3,56},
                                   {52,2,56},
                                   {32,5,64},	// Index 14
                                   {42,4,64},
                                   {48,3,64},
                                   {58,2,64},
                                   {70,1,64},
                                   {40,5,80},	// Index 19
                                   {52,4,80},
                                   {58,3,80},
                                   {70,2,80},
                                   {84,1,80},
                                   {48,5,96},	// Index 24
                                   {58,4,96},
                                   {70,3,96},
                                   {84,2,96},
                                   {104,1,96},
                                   {58,5,112},	// Index 29
                                   {70,4,112},
                                   {84,3,112},
                                   {104,2,112},
                                   {64,5,128},
                                   {84,4,128},	// Index 34
                                   {96,3,128},
                                   {116,2,128},
                                   {140,1,128},
                                   {80,5,160},
                                   {104,4,160},	// Index 39
                                   {116,3,160},
                                   {140,2,160},
                                   {168,1,160},
                                   {96,5,192},
                                   {116,4,192},	// Index 44
                                   {140,3,192},
                                   {168,2,192},
                                   {208,1,192},
                                   {116,5,224},
                                   {140,4,224},	// Index 49
                                   {168,3,224},
                                   {208,2,224},
                                   {232,1,224},
                                   {128,5,256},
                                   {168,4,256},	// Index 54
                                   {192,3,256},
                                   {232,2,256},
                                   {280,1,256},
                                   {160,5,320},
                                   {208,4,320},	// index 59
                                   {280,2,320},
                                   {192,5,384},
                                   {280,3,384},
                                   {416,1,384}};

//
	fib_processor::fib_processor (API_struct *p,
	                              void	*userData) {
	this	-> theParameters	= p;
	if (p -> ensemblename_Handler == nullptr) 
	   fprintf (stderr, "ensemblename_Handler is nullptr\n");
	this	-> ensemblenameHandler	= p ->  ensemblename_Handler;
	if (p -> programname_Handler == nullptr)
	   fprintf (stderr, "programname handler nullptr detected\n");
	this	-> programnameHandler	= p ->  programname_Handler;
	this	-> userData		= userData;
	memset (dateTime, 0, 8 * sizeof (uint32_t));

	reset	();
}
	
	fib_processor::~fib_processor () {
}
//
//	FIB's are segments of 256 bits. When here, they already
//	passed the crc and we start unpacking into FIGs
//	This is merely a dispatcher
void	fib_processor::process_FIB (uint8_t *p, uint16_t fib) {
uint8_t	FIGtype;
int8_t	processedBytes	= 0;
uint8_t	*d		= p;

	fibLocker. lock ();
	(void)fib;
	while (processedBytes  < 30) {
	   FIGtype 		= getBits_3 (d, 0);
	   uint8_t FIGlength    = getBits_5 (d, 3);
           if ((FIGtype == 0x07) && (FIGlength == 0x3F))
              return;

	   switch (FIGtype) {
	      case 0:
	         process_FIG0 (d);	
	         break;

	      case 1:
	         process_FIG1 (d);
	         break;

	      case 7:
	         break;

	      default:
//	         fprintf (stderr, "FIG%d aanwezig\n", FIGtype);
	         break;
	   }
//
//	Thanks to Ronny Kunze, who discovered that I used
//	a p rather than a d
	   processedBytes += getBits_5 (d, 3) + 1;
//	   processedBytes += getBits (p, 3, 5) + 1;
	   d = p + processedBytes * 8;
	}
	fibLocker. unlock ();
}
//
//	Handle ensemble is all through FIG0
//
void	fib_processor::process_FIG0 (uint8_t *d) {
uint8_t	extension	= getBits_5 (d, 8 + 3);
//uint8_t	CN	= getBits_1 (d, 8 + 0);

	switch (extension) {
	   case 0:
	      FIG0Extension0 (d);
	      break;

	   case 1:
	      FIG0Extension1 (d);
	      break;

	   case 2:
	      FIG0Extension2 (d);
	      break;

	   case 3:
	      FIG0Extension3 (d);
	      break;

	   case 4:
	      FIG0Extension4 (d);
	      break;

	   case 5:
	      FIG0Extension5 (d);
	      break;

	   case 6:
	      FIG0Extension6 (d);
	      break;

	   case 7:
	      FIG0Extension7 (d);
	      break;

	   case 8:
	      FIG0Extension8 (d);
	      break;

	   case 9:
	      FIG0Extension9 (d);
	      break;

	   case 10:
	      FIG0Extension10 (d);
	      break;

	   case 11:
	      FIG0Extension11 (d);
	      break;

	   case 12:
	      FIG0Extension12 (d);
	      break;

	   case 13:
	      FIG0Extension13 (d);
	      break;

	   case 14:
	      FIG0Extension14 (d);
	      break;

	   case 15:
	      FIG0Extension14 (d);
	      break;

	   case 16:
	      FIG0Extension16 (d);
	      break;

	   case 17:
	      FIG0Extension17 (d);
	      break;

	   case 18:
	      FIG0Extension18 (d);
	      break;

	   case 19:
	      FIG0Extension19 (d);
	      break;

	   case 20:
	      FIG0Extension20 (d);
	      break;

	   case 21:
	      FIG0Extension21 (d);
	      break;

	   case 22:
	      FIG0Extension22 (d);
	      break;

	   case 23:
	      FIG0Extension23 (d);
	      break;

	   case 24:
	      FIG0Extension24 (d);
	      break;

	   case 25:
	      FIG0Extension25 (d);
	      break;

	   case 26:
	      FIG0Extension26 (d);
	      break;

	   default:
//	      fprintf (stderr, "FIG0/%d passed by\n", extension);
	      break;
	}
}

//	Ensemble 6.4.1
//	FIG0/0 indicated a change in channel organization
//	we are not equipped for that, so we just return
//	control to the init
void	fib_processor::FIG0Extension0 (uint8_t *d) {
uint16_t	EId;
uint8_t		changeflag;
uint16_t	highpart, lowpart;
int16_t		occurrenceChange;
uint8_t	CN	= getBits_1 (d, 8 + 0);

	(void)CN;
	changeflag	= getBits_2 (d, 16 + 16);
	if (changeflag != 0)
	   return;

	EId			= getBits (d, 16, 16);
	(void)EId;
	highpart		= getBits_5 (d, 16 + 19) % 20;
	(void)highpart;
	lowpart			= getBits_8 (d, 16 + 24) % 250;
	(void)lowpart;
	occurrenceChange	= getBits_8 (d, 16 + 32);
	(void)occurrenceChange;

	CIFcount. store (highpart * 250 + lowpart);
	if (getBits (d, 34, 1))         // only alarm, just ignore
	   return;

//	if (changeflag == 1) {
//	   fprintf (stderr, "Changes in sub channel organization\n");
//	   fprintf (stderr, "cifcount = %d\n", highpart * 250 + lowpart);
//	   fprintf (stderr, "Change happening in %d CIFs\n", occurrenceChange);
//	}
//	else if (changeflag == 3) {
//	   fprintf (stderr, "Changes in subchannel and service organization\n");
//	   fprintf (stderr, "cifcount = %d\n", highpart * 250 + lowpart);
//	   fprintf (stderr, "Change happening in %d CIFs\n", occurrenceChange);
//	}
//	fprintf (stderr, "changes in config not supported, choose again\n");
//	emit  changeinConfiguration ();
//
}
//
//	Subchannel organization 6.2.1
//	FIG0 extension 1 creates a mapping between the
//	sub channel identifications and the positions in the
//	relevant CIF.
void	fib_processor::FIG0Extension1 (uint8_t *d) {
int16_t	used	= 2;		// offset in bytes
int16_t	Length	= getBits_5 (d, 3);
uint8_t	PD_bit	= getBits_1 (d, 8 + 2);
//uint8_t	CN	= getBits_1 (d, 8 + 0);

	while (used < Length - 1)
	   used = HandleFIG0Extension1 (d, used, PD_bit);
}
//
//	defining the channels 
int16_t	fib_processor::HandleFIG0Extension1 (uint8_t *d,
	                                     int16_t offset,
	                                     uint8_t pd) {
int16_t	bitOffset	= offset * 8;
int16_t	SubChId		= getBits_6 (d, bitOffset);
int16_t StartAdr	= getBits (d, bitOffset + 6, 10);
int16_t	tabelIndex;
int16_t	option, protLevel, subChanSize;
static  int table_1 [] = {12, 8, 6, 4};
static  int table_2 [] = {27, 21, 18, 15};

	subChannels [SubChId]. StartAddr = StartAdr;
	subChannels [SubChId]. inUse	 = true;

	if (getBits_1 (d, bitOffset + 16) == 0) {	// short form
	   tabelIndex = getBits_6 (d, bitOffset + 18);
	   subChannels [SubChId]. Length  	= ProtLevel [tabelIndex][0];
	   subChannels [SubChId]. shortForm	= true;
	   subChannels [SubChId]. protLevel	= ProtLevel [tabelIndex][1];
	   subChannels [SubChId]. BitRate	= ProtLevel [tabelIndex][2];
	   bitOffset += 24;
	}
	else { 	// EEP long form
	   subChannels [SubChId]. shortForm	= false;
	   option = getBits_3 (d, bitOffset + 17);
	   if (option == 0) { 		// A Level protection
	      protLevel = getBits_2 (d, bitOffset + 20);
//
	      subChannels [SubChId]. protLevel = protLevel;
	      subChanSize = getBits (d, bitOffset + 22, 10);
	      subChannels [SubChId]. Length	= subChanSize;
	      subChannels [SubChId]. BitRate	= subChanSize / table_1 [protLevel] * 8;
	   }
	   else			// option should be 001
	   if (option == 001) {		// B Level protection
	      protLevel				=
	                                 getBits_2 (d, bitOffset + 20);
	      subChannels [SubChId]. protLevel= protLevel + (1 << 2);
	      subChanSize			=
	                                 getBits (d, bitOffset + 22, 10);
	      subChannels [SubChId]. Length	=
	                                 subChanSize;
	      subChannels [SubChId]. BitRate	= 
	                                 subChanSize /table_2 [protLevel] * 32;
	   }

	   bitOffset += 32;
	}
	return bitOffset / 8;	// we return bytes
}
//
//	Service organization, 6.3.1
//	bind channels to serviceIds
void	fib_processor::FIG0Extension2 (uint8_t *d) {
int16_t	used	= 2;		// offset in bytes
int16_t	Length	= getBits_5 (d, 3);
uint8_t	PD_bit	= getBits_1 (d, 8 + 2);
uint8_t	CN	= getBits_1 (d, 8 + 0);

	while (used < Length) {
	   used = HandleFIG0Extension2 (d, used, CN, PD_bit);
	}
}
//
//	Note Offset is in bytes
int16_t	fib_processor::HandleFIG0Extension2 (uint8_t *d,
	                                     int16_t offset,
	                                     uint8_t cn,
	                                     uint8_t pd) {
int16_t		lOffset	= 8 * offset;
int16_t		i;
uint8_t		ecc;
uint8_t		cId;
uint32_t	SId;
int16_t		numberofComponents;

	if (pd == 1) {		// long Sid
	   ecc	= getBits_8 (d, lOffset);	(void)ecc;
	   cId	= getBits_4 (d, lOffset + 1);
	   SId	= getLBits (d, lOffset, 32);
	   lOffset	+= 32;
	}
	else {
	   cId	= getBits_4 (d, lOffset);	(void)cId;
	   SId	= getBits (d, lOffset + 4, 12);
	   SId	= getBits (d, lOffset, 16);
	   lOffset	+= 16;
	}

	numberofComponents	= getBits_4 (d, lOffset + 4);
	lOffset	+= 8;

	for (i = 0; i < numberofComponents; i ++) {
	   uint8_t	TMid	= getBits_2 (d, lOffset);
	   if (TMid == 00)  {	// Audio
	      uint8_t	ASCTy	= getBits_6 (d, lOffset + 2);
	      uint8_t	SubChId	= getBits_6 (d, lOffset + 8);
	      uint8_t	PS_flag	= getBits_1 (d, lOffset + 14);
	      bind_audioService (TMid, SId, i, SubChId, PS_flag, ASCTy);
	   }
	   else
	   if (TMid == 3) { // MSC packet data
	      int16_t SCId	= getBits (d, lOffset + 2, 12);
	      uint8_t PS_flag	= getBits_1 (d, lOffset + 14);
	      uint8_t CA_flag	= getBits_1 (d, lOffset + 15);
	      bind_packetService (TMid, SId, i, SCId, PS_flag, CA_flag);
           }
	   else
	      {;}		// for now
	   lOffset += 16;
	}
	return lOffset / 8;		// in Bytes
}
//
//	Service component in packet mode 6.3.2
//      The Extension 3 of FIG type 0 (FIG 0/3) gives
//      additional information about the service component
//      description in packet mode.
//      manual: page 55
void	fib_processor::FIG0Extension3 (uint8_t *d) {
int16_t	used	= 2;
int16_t	Length	= getBits_5 (d, 3);

	while (used < Length)
	   used = HandleFIG0Extension3 (d, used);
}

//
//      DSCTy   DataService Component Type
int16_t fib_processor::HandleFIG0Extension3 (uint8_t *d, int16_t used) {
int16_t	SCId            = getBits (d, used * 8, 12);
int16_t CAOrgflag       = getBits_1 (d, used * 8 + 15);
int16_t DGflag          = getBits_1 (d, used * 8 + 16);
int16_t DSCTy           = getBits_6 (d, used * 8 + 18);
int16_t SubChId         = getBits_6 (d, used * 8 + 24);
int16_t packetAddress   = getBits (d, used * 8 + 30, 10);
uint16_t CAOrg;

serviceComponent *packetComp = find_packetComponent (SCId);
serviceId	 *service;

	if (CAOrgflag == 1) {
	   CAOrg = getBits (d, used * 8 + 40, 16);
	   used += 16 / 8; 
        }
        used += 40 / 8;
	(void)CAOrg;
        if (packetComp == nullptr)		// no serviceComponent yet
           return used;

//      We want to have the subchannel OK
	if (!subChannels [SubChId]. inUse)
	   return used;

//      If the component exists, we first look whether is
//      was already handled
        if (packetComp -> is_madePublic)
           return used;
//
//      We want to have the subchannel OK
        if (!subChannels [SubChId]. inUse)
           return used;

//      if the  Data Service Component Type == 0, we do not deal
//      with it
        if (DSCTy == 0)
           return used;

	service = packetComp -> service;
        std::string serviceName = service -> serviceLabel. label;
        if (packetComp -> componentNr == 0)     // otherwise sub component
           addtoEnsemble (serviceName, service -> serviceId);

        packetComp      -> is_madePublic = true;
        packetComp      -> subchannelId = SubChId;
        packetComp      -> DSCTy        = DSCTy;
	packetComp	-> DGflag	= DGflag;
        packetComp      -> packetAddress        = packetAddress;
        return used;
}
//
//      Service component with CA in stream mode 6.3.3
void    fib_processor::FIG0Extension4 (uint8_t *d) {
int16_t used    = 3;            // offset in bytes
int16_t Rfa     = getBits_1 (d, 0);
int16_t Rfu     = getBits_1 (d, 0 + 1);
int16_t SubChId = getBits_6 (d, 0 + 1 + 1);
int32_t CAOrg = getBits (d, 2 + 6, 16);

//      fprintf(stderr,"FIG0/4: Rfa=\t%D, Rfu=\t%d, SudChId=\t%02X, CAOrg=\t%04X\n", Rfa, Rfu, SubChId, CAOrg);
	(void)used; (void)Rfa; (void)Rfu; (void)SubChId; (void)CAOrg;
}
//
//	Service component language
void	fib_processor::FIG0Extension5 (uint8_t *d) {
int16_t	used	= 2;		// offset in bytes
int16_t	Length	= getBits_5 (d, 3);

	while (used < Length) {
	   used = HandleFIG0Extension5 (d, used);
	}
}

int16_t	fib_processor::HandleFIG0Extension5 (uint8_t* d, int16_t offset) {
int16_t	loffset	= offset * 8;
uint8_t	lsFlag	= getBits_1 (d, loffset);
int16_t	subChId, serviceComp, language;

	if (lsFlag == 0) {	// short form
	   if (getBits_1 (d, loffset + 1) == 0) {
	      subChId	= getBits_6 (d, loffset + 2);
	      language	= getBits_8 (d, loffset + 8);
	      subChannels [subChId]. language = language;
	   }
	   loffset += 16;
	}
	else {			// long form
	   serviceComp	= getBits (d, loffset + 4, 12);
	   language	= getBits_8 (d, loffset + 16);
	   loffset += 24;
	}
	(void)serviceComp;
	return loffset / 8;
}

// FIG0/6: Service linking information 8.1.15, not implemented
void    fib_processor::FIG0Extension6 (uint8_t *d) {
}

// FIG0/7: Configuration linking information 6.4.2, not implemented
void    fib_processor::FIG0Extension7 (uint8_t *d) {
}

void	fib_processor::FIG0Extension8 (uint8_t *d) {
int16_t	used	= 2;		// offset in bytes
int16_t	Length	= getBits_5 (d, 3);
uint8_t	PD_bit	= getBits_1 (d, 8 + 2);

	while (used < Length) {
	   used = HandleFIG0Extension8 (d, used, PD_bit);
	}
}

int16_t	fib_processor::HandleFIG0Extension8 (uint8_t *d, int16_t used,
	                                     uint8_t pdBit) {
int16_t	lOffset	= used * 8;
uint32_t	SId	= getLBits (d, lOffset, pdBit == 1 ? 32 : 16);
uint8_t		lsFlag;
uint16_t	SCIds;
int16_t		SCid;
int16_t		MSCflag;
int16_t		SubChId;
uint8_t		extensionFlag;

	lOffset += pdBit == 1 ? 32 : 16;
        extensionFlag   = getBits_1 (d, lOffset);
        SCIds   = getBits_4 (d, lOffset + 4);
        lOffset += 8;

        lsFlag  = getBits_1 (d, lOffset);
        if (lsFlag == 1) {
           SCid = getBits (d, lOffset + 4, 12);
           lOffset += 16;
        }
	else {
	   MSCflag	= getBits_1 (d, lOffset + 1);
	   SubChId	= getBits_6 (d, lOffset + 2);
	   lOffset += 8;
	}
	if (extensionFlag)
	   lOffset += 8;	// skip Rfa
	(void)SId;
	(void)SCIds;
	(void)SCid;
	(void)SubChId;
	(void)MSCflag;
	return lOffset / 8;
}
//
//	Country, LTO & international table 8.1.3.2
//	FIG0/9 and FIG0/10 are copied from the work of
//	Michael Hoehn
void fib_processor::FIG0Extension9 (uint8_t *d) {
int16_t	offset	= 16;
int     signbit = getBits_1 (d, offset + 2);

	dateTime [6] = (getBits_1 (d, offset + 2) == 1)?
	                -1 * getBits_4 (d, offset + 3):
	                     getBits_4 (d, offset + 3);
//	7 indicates a possible remaining half our
	dateTime [7] = (getBits_1 (d, offset + 7) == 1) ? 30 : 0;
	if (signbit == 1)
	   dateTime [7] = -dateTime [7];
}

std::string monthTable [] = {
"jan", "feb", "mar", "apr", "may", "jun",
"jul", "aug", "sep", "oct", "nov", "dec"};

int	monthLength [] {
31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
//
//	Time in 10 is given in UTC, for other time zones
//	we add (or subtract) a number of Hours (half hours)
void	adjustTime (int32_t *dateTime) {
//	first adjust the half hour  in the amount of minutes
	dateTime [4] += (dateTime [7] == 1) ? 30 : 0;
	if (dateTime [4] >= 60) {
	   dateTime [4] -= 60; dateTime [3] ++;
	}

	if (dateTime [4] < 0) {
	   dateTime [4] += 60; dateTime [3] --;
	}

	dateTime [3] += dateTime [6];
	if ((0 <= dateTime [3]) && (dateTime [3] <= 23))
	   return;

	if (dateTime [3] > 23) {
	   dateTime [3] -= 24; dateTime [2] ++;
	}

	if (dateTime [3] < 0) {
	   dateTime [3] += 24; dateTime [2] --;
	}

	if (dateTime [2] > monthLength [dateTime [1] - 1]) {
	   dateTime [2] = 1; dateTime [1] ++;
	   if (dateTime [1] > 12) {
	      dateTime [1] = 1;
	      dateTime [0] ++;
	   }
	}

	if (dateTime [2] < 0) {
	   if (dateTime [1] > 1) {
	      dateTime [2] = monthLength [dateTime [1] - 1 - 1];
	      dateTime [1] --;
	   }
	   else {
	      dateTime [2] = monthLength [11];
	      dateTime [1] = 12; dateTime [0] --;
	   }
	}
}

std::string	mapTime (int32_t *dateTime) {
std::string result;
char temp [20];
	int hours	= dateTime [3];
	if (hours < 0)	hours += 24;
	if (hours >= 24) hours -= 24;

	sprintf (temp, "%2d", hours);
	std::string hoursasString = temp;
	result. append (hoursasString);
	result. append (":");
	sprintf (temp, "%2d", dateTime [4]);
	std::string minutesasString = temp;
	result. append (minutesasString);
	return result;
}
//
void fib_processor::FIG0Extension10 (uint8_t *dd) {
int16_t		offset = 16;
int32_t		mjd	= getLBits (dd, offset + 1, 17);
// Modified Julian Date umrechnen (Nach wikipedia)
int32_t J	= mjd + 2400001;
int32_t j	= J + 32044;
int32_t g	= j / 146097; 
int32_t	dg	= j % 146097;
int32_t c	= ((dg / 36524) + 1) * 3 / 4; 
int32_t dc	= dg - c * 36524;
int32_t b	= dc / 1461;
int32_t db	= dc%1461;
int32_t a	= ((db / 365) + 1) * 3 / 4; 
int32_t da	= db - a * 365;
int32_t y	= g * 400 + c * 100 + b * 4 + a;
int32_t m	= ((da * 5 + 308) / 153) - 2;
int32_t d	= da - ((m + 4) * 153 / 5) + 122;
int32_t Y	= y - 4800 + ((m + 2) / 12); 
int32_t M	= ((m + 2) % 12) + 1; 
int32_t D	= d + 1;
uint32_t	theTime [6];

	theTime [0] = Y;	// Year
	theTime [1] = M;	// Month
	theTime [2] = D;	// Day
	theTime [3] = getBits_5 (dd, offset + 21); // Hours
	theTime [4] = getBits_6 (dd, offset + 26); // Minutes

	if (getBits_6 (dd, offset + 26) != dateTime [4]) 
	   theTime [5] =  0;	// Seconds (Ubergang abfangen)

	if (dd [offset + 20] == 1)
	   theTime [5] = getBits_6 (dd, offset + 32);	// Seconds
//
//	take care of different time zones
	bool	change = false;
	for (int i = 0; i < 5; i ++) {
	   if (theTime [i] != dateTime [i])
	      change = true;
	   dateTime [i] = theTime [i];
	}

	if (change && theParameters -> timeHandler != nullptr) {
	   adjustTime (dateTime);
//	   std::string timeString = mapTime (dateTime);
//	   theParameters ->  timeHandler (dateTime [3],
//	                                  dateTime [4], userData);
	}
}
//
//
void    fib_processor::FIG0Extension11 (uint8_t *d) {
        (void)d;
}
//
//
void    fib_processor::FIG0Extension12 (uint8_t *d) {
        (void)d;
}
//
//
void	fib_processor::FIG0Extension13 (uint8_t *d) {
int16_t	used	= 2;		// offset in bytes
int16_t	Length	= getBits_5 (d, 3);
uint8_t	PD_bit	= getBits_1 (d, 8 + 2);

	while (used < Length) 
	   used = HandleFIG0Extension13 (d, used, PD_bit);
}

int16_t	fib_processor::HandleFIG0Extension13 (uint8_t *d,
	                                      int16_t used,
	                                      uint8_t pdBit) {
int16_t	lOffset		= used * 8;
uint32_t	SId	= getLBits (d, lOffset, pdBit == 1 ? 32 : 16);
uint16_t	SCIdS;
int16_t		NoApplications;
int16_t		i;
int16_t		appType;

	lOffset		+= pdBit == 1 ? 32 : 16;
	SCIdS		= getBits_4 (d, lOffset);
	NoApplications	= getBits_4 (d, lOffset + 4);
	lOffset += 8;

	for (i = 0; i < NoApplications; i ++) {
	   appType		= getBits (d, lOffset, 11);
	   int16_t length	= getBits_5 (d, lOffset + 11);
	   lOffset += (11 + 5 + 8 * length);
	   serviceComponent *packetComp        =
	                         find_serviceComponent (SId, SCIdS);
	   if (packetComp != nullptr) 
	      packetComp      -> appType       = appType;
	}

	return lOffset / 8;
}
//
//      FEC sub-channel organization 6.2.2
void	fib_processor::FIG0Extension14 (uint8_t *d) {
int16_t	Length	= getBits_5 (d, 3);	// in Bytes
int16_t	used	= 2;			// in Bytes

	while (used < Length) {
	   int16_t SubChId	= getBits_6 (d, used * 8);
	   uint8_t FEC_scheme	= getBits_2 (d, used * 8 + 6);
	   used = used + 1;
	   if (subChannels [SubChId]. inUse)
	      subChannels [SubChId]. FEC_scheme = FEC_scheme;
	}
}

void    fib_processor::FIG0Extension15 (uint8_t *d) {
        (void)d;
}
//
//      Obsolete in ETSI EN 300 401 V2.1.1 (2017-01)
void	fib_processor::FIG0Extension16 (uint8_t *d) {
int16_t	length 	= getBits_5 (d, 3);	// in bytes
int16_t	offset	= 16;			// in bits
serviceId	*s;

	while (offset < length * 8) {
	   uint16_t	SId	= getBits (d, offset, 16);
	   s	= findServiceId (SId);
	   if (!s -> hasPNum) {
	      uint8_t PNum = getBits (d, offset + 16, 16);
	      s -> pNum		= PNum;
	      s -> hasPNum	= true;
//	      fprintf (stderr, "Program number info SId = %.8X, PNum = %d\n",
//	      	                               SId, PNum);
	   }
	   offset += 72;
	}
}
//
//      Programme Type (PTy) 8.1.5
void	fib_processor::FIG0Extension17 (uint8_t *d) {
int16_t	length	= getBits_5 (d, 3);
int16_t	offset	= 16;
serviceId	*s;

	while (offset < length * 8) {
	   uint16_t	SId	= getBits (d, offset, 16);
	   bool	L_flag	= getBits_1 (d, offset + 18);
	   bool	CC_flag	= getBits_1 (d, offset + 19);
	   int16_t type;
	   int16_t Language = 0x00;	// init with unknown language
	   s	= findServiceId (SId);
	   if (L_flag) {		// language field present
	      Language = getBits_8 (d, offset + 24);
	      s -> language = Language;
	      s -> hasLanguage = true;
	      offset += 8;
	   }

	   type	= getBits_5 (d, offset + 27);
	   s	-> programType	= type;
	   if (CC_flag)			// cc flag
	      offset += 40;
	   else
	      offset += 32;
	}
}
//
//      Announcement support 8.1.6.1
void	fib_processor::FIG0Extension18 (uint8_t *d) {
int16_t	offset	= 16;		// bits
uint16_t	SId, AsuFlags;
int16_t		Length	= getBits_5 (d, 3);

	while (offset / 8 < Length - 1 ) {
	   int16_t NumClusters = getBits_5 (d, offset + 35);
	   SId	= getBits (d, offset, 16);
	   AsuFlags	= getBits (d, offset + 16, 16);
//	   fprintf (stderr, "Announcement %d for SId %d with %d clusters\n",
//	                    AsuFlags, SId, NumClusters);
	   offset += 40 + NumClusters * 8;
	}
	(void)SId;
	(void)AsuFlags;
}
//
//      Announcement switching 8.1.6.2
void	fib_processor::FIG0Extension19 (uint8_t *d) {
int16_t		offset	= 16;		// bits
uint16_t	AswFlags;
int16_t		Length	= getBits_5 (d, 3);
uint8_t		region_Id_Lower;

	while (offset / 8 < Length - 1) {
	   uint8_t ClusterId	= getBits_8 (d, offset);
	   bool new_flag	= getBits_1(d, offset + 24);
	   bool	region_flag	= getBits_1 (d, offset + 25);
	   uint8_t SubChId	= getBits_6 (d, offset + 26);

	   AswFlags	= getBits (d, offset + 8, 16);
//	   fprintf (stderr,
//	          "%s %s Announcement %d for Cluster %2u on SubCh %2u ",
//	              ((new_flag==1)?"new":"old"),
//	              ((region_flag==1)?"regional":""),
//	              AswFlags, ClusterId,SubChId);
	   if (region_flag) {
	      region_Id_Lower = getBits_6 (d, offset + 34);
	      offset += 40;
//           fprintf(stderr,"for region %u",region_Id_Lower);
	   }
	   else
	      offset += 32;

//	   fprintf(stderr,"\n");
	   (void)ClusterId;
	   (void)new_flag;
	   (void)SubChId;
	}
	(void)AswFlags;
	(void)region_Id_Lower;
}
//
//      Service component information 8.1.4
void    fib_processor::FIG0Extension20 (uint8_t *d) {
        (void)d;
}
//
//	Frequency information (FI) 8.1.8
void	fib_processor::FIG0Extension21 (uint8_t *d) {
//	fprintf (stderr, "Frequency information\n");
	(void)d;
}
//
//      Obsolete in ETSI EN 300 401 V2.1.1 (2017-01)
void	fib_processor::FIG0Extension22 (uint8_t *d) {
int16_t	Length	= getBits_5 (d, 3);
int16_t	offset	= 16;		// on bits
int16_t	used	= 2;

	while (used < Length) 
	   used = HandleFIG0Extension22 (d, used);
	(void)offset;
}

int16_t	fib_processor::HandleFIG0Extension22 (uint8_t *d, int16_t used) {
	(void)d; (void)used;
	return 0;
}
//
//
void    fib_processor::FIG0Extension23 (uint8_t *d) {
        (void)d;
}
//
//      OE Services
void    fib_processor::FIG0Extension24 (uint8_t *d) {
        (void)d;
}
//
//      OE Announcement support
void    fib_processor::FIG0Extension25 (uint8_t *d) {
        (void)d;
}
//
//      OE Announcement Switching
void    fib_processor::FIG0Extension26 (uint8_t *d) {
        (void)d;
}

//      FIG 1 - Cover the different possible labels, section 5.2
//
void	fib_processor::process_FIG1 (uint8_t *d) {
uint8_t		charSet, extension;
uint32_t	SId	= 0;
uint8_t		Rfu;
int16_t		offset	= 0;
serviceId	*myIndex;
int16_t		i;
uint8_t		pd_flag;
uint8_t		SCidS;
uint8_t		XPAD_aid;
uint8_t		flagfield;
uint8_t		region_id;
char		label [17];
//
//	from byte 1 we deduce:
	charSet		= getBits_4 (d, 8);
	Rfu		= getBits_1 (d, 8 + 4);
	extension	= getBits_3 (d, 8 + 5); 
	label [16] = 0;
	(void)Rfu;
	switch (extension) {
/*
	   default:
	      return;
 */
	   case 0:	// ensemble label
	      SId	= getBits (d, 16, 16);
	      offset	= 32;
	      if ((charSet <= 16)) { // EBU Latin based repertoire
	         for (i = 0; i < 16; i ++) {
	            label [i] = getBits_8 (d, offset + 8 * i);
	         }
//	         fprintf (stderr, "Ensemblename: %16s (%d)\n", label, charSet);
	         {
	            std::string name = toStringUsingCharset (
	                                      (const char *) label,
	                                      (CharacterSet) charSet);
	            if (firstTime)
	               nameofEnsemble (SId, name);
	            firstTime	= false;
	            isSynced	= true;
	         }
	      }
//	      fprintf (stderr,
//	               "charset %d is used for ensemblename\n", charSet);
	      break;

	   case 1:	// 16 bit Identifier field for service label 8.1.14.1
	      SId	= getBits (d, 16, 16);
	      offset	= 32;
	      myIndex	= findServiceId (SId);
	      if ((!myIndex -> serviceLabel. hasName) && (charSet <= 16)) {
	         for (i = 0; i < 16; i ++) {
	            label [i] = getBits_8 (d, offset + 8 * i);
	         }

	         myIndex -> serviceLabel. label. append (
	                       toStringUsingCharset (
	                                (const char *) label,
	                                (CharacterSet) charSet));
//	         fprintf (stderr, "FIG1/1: SId = %4x\t%s\n", SId, label);
	         myIndex -> serviceLabel. hasName = true;
	      }
	      break;

	   case 3:
	      // region label
	      region_id = getBits_6 (d, 16 + 2);
	      (void)region_id;
	      offset = 24;
	      for (i = 0; i < 16; i ++) 
	         label [i] = getBits_8 (d, offset + 8 * i);

//	      fprintf (stderr, "FIG1/3: RegionID = %2x\t%s\n", region_id, label);
	      break;

	   case 4:	 // service component label 8.1.14.3
	      pd_flag	= getLBits (d, 16, 1);
	      SCidS	= getLBits (d, 20, 4);
	      if (pd_flag) {	// 32 bit identifier field for service component label
	         SId	= getLBits (d, 24, 32);
	         offset	= 56;
	      }
	      else {	// 16 bit identifier field for service component label
	         SId	= getLBits (d, 24, 16);
	         offset	= 40;
	      }

	      flagfield	= getLBits (d, offset + 128, 16);
	      for (i = 0; i < 16; i ++)
	         label [i] = getBits_8 (d, offset + 8 * i);
//	      fprintf (stderr, "FIG1/4: Sid = %8x\tp/d=%d\tSCidS=%1X\tflag=%8X\t%s\n",
//	                        SId, pd_flag, SCidS, flagfield, label);
	      break;


	   case 5:	 // Data service label - 32 bits 8.1.14.2
	      SId	= getLBits (d, 16, 32);
	      offset	= 48;
	      myIndex   = findServiceId (SId);
              if ((!myIndex -> serviceLabel. hasName) && (charSet <= 16)) {
                 for (i = 0; i < 16; i ++) {
                    label [i] = getBits_8 (d, offset + 8 * i);
                 }
                 myIndex -> serviceLabel. label. append (
                               toStringUsingCharset (
                                         (const char *) label,
                                         (CharacterSet) charSet));
	         myIndex -> serviceLabel. label. append (
                               toStringUsingCharset (
	                                 " (data)",
                                         (CharacterSet) charSet));
                 myIndex -> serviceLabel. hasName = true;
	         addtoEnsemble (myIndex -> serviceLabel. label, SId);
              }
	      break;

	   case 6:	// XPAD label - 8.1.14.4
	      pd_flag	= getLBits (d, 16, 1);
	      SCidS	= getLBits (d, 20, 4);
	      if (pd_flag) {	// 32 bits identifier for XPAD label
	         SId		= getLBits (d, 24, 32);
	         XPAD_aid	= getLBits (d, 59, 5);
	         offset 	= 64;
	      }
	      else {	// 16 bit identifier for XPAD label
	         SId		= getLBits (d, 24, 16);
	         XPAD_aid	= getLBits (d, 43, 5);
	         offset		= 48;
	      }

	      for (i = 0; i < 16; i ++)
	         label [i] = getBits_8 (d, offset + 8 * i);

//	      fprintf (stderr, "FIG1/6: SId = %8x\tp/d = %d\t SCidS = %1X\tXPAD_aid = %2u\t%s\n",
//		       SId, pd_flag, SCidS, XPAD_aid, label);
	      break;

	   default:
//	      fprintf (stderr, "FIG1/%d: not handled now\n", extension);
	      break;
	}
	(void)SCidS;
	(void)XPAD_aid;
	(void)flagfield;
}

#define	FULL_MATCH	0100
#define	PREFIX_MATCH	0200
#define	NO_MATCH	0000

//	tricky: the names in the directoty contain spaces at the end
static
int	compareNames (const std::string in, const std::string ref) {

	if (ref == in)
	   return FULL_MATCH;

	if (ref. length () < in. length ())
	   return NO_MATCH;

	if (ref. find (in, 0) != 0) 
	   return NO_MATCH;

	if  (in. length () == ref. length ())
	   return FULL_MATCH;
//
//	Most likely we will find a prefix as match, since the
//	FIC structure fills the service names woth spaces to 16 letters
	if (ref. at (in. length ()) == ' ')
	   return FULL_MATCH;

	return PREFIX_MATCH;
}

//	locate - and create if needed - a reference to the entry
//	for the serviceId serviceId
serviceId	*fib_processor::findServiceId (int32_t serviceId) {
int16_t	i;

	for (i = 0; i < 64; i ++)
	   if ((listofServices [i]. inUse) &&
	        (listofServices [i]. serviceId == (uint32_t)serviceId))
	      return &listofServices [i];

	for (i = 0; i < 64; i ++)
	   if (!listofServices [i]. inUse) {
	      listofServices [i]. inUse = true;
	      listofServices [i]. serviceLabel. hasName = false;
	      listofServices [i]. serviceId = serviceId;
	      listofServices [i]. language = -1;
	      return &listofServices [i];
	   }

	return &listofServices [0];	// should not happen
}
//
//	since some servicenames are long, we allow selection of a
//	service based on the first few letters/digits of the name.
//	However, in case of servicenames where one is a prefix
//	of the other, the full match should have precedence over the
//	prefix match
serviceId	*fib_processor::findServiceId (const std::string serviceName) {
int16_t	i;
int	indexforprefixMatch	= -1;

	for (i = 0; i < 64; i ++) {
	   if (listofServices [i]. inUse) {
	      int res = compareNames (serviceName,
	                     listofServices [i]. serviceLabel. label);
	      if (res == FULL_MATCH) {
	         return &listofServices [i];
	      }
	      if (res == PREFIX_MATCH) {
	         indexforprefixMatch = i;
	      }
	   }
	}

	return indexforprefixMatch >= 0 ?
	              &listofServices [indexforprefixMatch] : nullptr;
}

serviceComponent *fib_processor::find_packetComponent (int16_t SCId) {
int16_t i;

        for (i = 0; i < 64; i ++) {
           if (!ServiceComps [i]. inUse)
              continue;
           if (ServiceComps [i]. TMid != 03)
              continue;
           if (ServiceComps [i]. SCId == SCId)
              return &ServiceComps [i];
        }
        return nullptr;
}

serviceComponent *fib_processor::find_serviceComponent (int32_t SId,
	                                                int16_t SCIdS) {
int16_t i;

	for (i = 0; i < 64; i ++) {
	   if (!ServiceComps [i]. inUse)
	      continue;

	   if ( (findServiceId (SId) == ServiceComps [i]. service)) {
	      return &ServiceComps [i];
	   }
	}

	return nullptr;
}

//	bind_audioService is the main processor for - what the name suggests -
//	connecting the description of audioservices to a SID
void	fib_processor::bind_audioService (int8_t TMid,
	                                  uint32_t SId,
	                                  int16_t compnr,
	                                  int16_t SubChId,
	                                  int16_t ps_flag,
	                                  int16_t ASCTy) {
serviceId *s	= findServiceId	(SId);
int16_t	i;
int16_t	firstFree	= -1;

	if (!s -> serviceLabel. hasName)
	   return;

	if (!subChannels [SubChId]. inUse)
	   return;

	for (i = 0; i < 64; i ++) {
	   if (!ServiceComps [i]. inUse) {
	      if (firstFree == -1)
	         firstFree = i;
	      continue;
	   }
	   if ((ServiceComps [i]. service == s) &&
               (ServiceComps [i]. componentNr == compnr))
	      return;
	}

	std::string dataName = s -> serviceLabel. label;
        addtoEnsemble (dataName, s -> serviceId);

	ServiceComps [firstFree]. inUse		= true;
	ServiceComps [firstFree]. TMid		= TMid;
	ServiceComps [firstFree]. componentNr	= compnr;
	ServiceComps [firstFree]. service	= s;
	ServiceComps [firstFree]. subchannelId = SubChId;
	ServiceComps [firstFree]. PS_flag	= ps_flag;
	ServiceComps [firstFree]. ASCTy		= ASCTy;
}

//      bind_packetService is the main processor for - what the name suggests -
//      connecting the service component defining the service to the SId,
///     Note that the subchannel is assigned through a FIG0/3
void    fib_processor::bind_packetService (int8_t TMid,
                                           uint32_t SId,
                                           int16_t compnr,
                                           int16_t SCId,
                                           int16_t ps_flag,
                                           int16_t CAflag) {
serviceId *s    = findServiceId (SId);
int16_t i;
int16_t	firstFree	= -1;

	if (!s -> serviceLabel. hasName)        // wait until we have a name
           return;

	for (i = 0; i < 64; i ++) {
	   if ((ServiceComps [i]. inUse) &&
	                      (ServiceComps [i]. SCId == SCId))
	      return;

	   if (!ServiceComps [i]. inUse) {
	      if (firstFree == -1)
	         firstFree = i;
	      continue;
	   }
	}

	ServiceComps [firstFree]. inUse		= true;
	ServiceComps [firstFree]. TMid		= TMid;
	ServiceComps [firstFree]. service	= s;
	ServiceComps [firstFree]. componentNr	= compnr;
	ServiceComps [firstFree]. SCId		= SCId;
	ServiceComps [firstFree]. PS_flag	= ps_flag;
	ServiceComps [firstFree]. CAflag	= CAflag;
	ServiceComps [firstFree]. is_madePublic = false;
}

void	fib_processor::setupforNewFrame () {
int16_t	i;
	isSynced	= false;
	for (i = 0; i < 64; i ++)
	   ServiceComps [i]. inUse = false;
}

void	fib_processor::clearEnsemble () {
int16_t i;

	setupforNewFrame ();
	memset (ServiceComps, 0, sizeof (ServiceComps));
	memset (subChannels, 0, sizeof (subChannels));
	for (i = 0; i < 64; i ++) {
	   listofServices [i]. inUse = false;
	   listofServices [i]. serviceId = -1;
	   listofServices [i]. serviceLabel. label = "";
	   ServiceComps [i]. inUse	= false;
	   subChannels [i]. inUse	= false;
	}
	firstTime	= true;
}

std::string fib_processor::nameFor (const int32_t serviceId) {
int16_t i;

	for (i = 0; i < 64; i ++) {
	   if (!listofServices [i]. inUse)
	      continue;

	   if (!listofServices [i]. serviceLabel. hasName)
              continue;

	   if (listofServices [i]. serviceId == (uint32_t) serviceId) {
	      return listofServices [i]. serviceLabel. label;
	   }
	}
	return "no service found";
}

int32_t	fib_processor::SIdFor (const std::string &name) {
int16_t i;
int	serviceIndex	= -1;

	for (i = 0; i < 64; i ++) {
	   if (!listofServices [i]. inUse)
	      continue;

	   if (!listofServices [i]. serviceLabel. hasName)
              continue;

	   int res = compareNames (name,
	                      listofServices [i]. serviceLabel. label);
	   if (res == NO_MATCH)
	      continue;
	   if (res == PREFIX_MATCH) {
	      serviceIndex = i;
	      continue;
	   }
//	it is a FULL match:
	   return listofServices [i]. serviceId;
	}
	if (serviceIndex >= 0)
	   return listofServices [serviceIndex]. serviceId;
	return -1;
}
//
//	Here we look for a primary service only
uint8_t	fib_processor::kindofService (const std::string &s) {
int16_t	i, j;
int16_t	service		= UNKNOWN_SERVICE;
int32_t	selectedService = -1;
int	serviceIndex	= -1;

	fibLocker. lock ();
//	first we locate the serviceId
	for (i = 0; i < 64; i ++) {
	   int	res;
	   if (!listofServices [i]. inUse)
	      continue;

	   if (!listofServices [i]. serviceLabel. hasName)
	      continue;

	   res = compareNames (s,
	                listofServices [i]. serviceLabel. label);
	   if (res == NO_MATCH)
	      continue;
	   if (res == PREFIX_MATCH) {
	      serviceIndex = i;
	      continue;
	   }
	   serviceIndex = i;
	   break;
	}

	if (serviceIndex != -1) {
	   selectedService = listofServices [serviceIndex]. serviceId;
	   for (j = 0; j < 64; j ++) {
	      if (!ServiceComps [j]. inUse)
	         continue;
	      if ((uint32_t)selectedService !=
	                           ServiceComps [j]. service -> serviceId)
	         continue;

	      if (ServiceComps [j]. componentNr != 0)
	         continue;

	      if (ServiceComps [j]. TMid == 03) {
	         service = PACKET_SERVICE;
	         break;
	      }

	      if (ServiceComps [j]. TMid == 00) {
	         service = AUDIO_SERVICE;
	         break;
	      }
	   }
	}
	fibLocker. unlock ();
	return service;
}

void	fib_processor::dataforDataService (const std::string &s,
	                                                packetdata *d) {
	dataforDataService (s, d, 0);
}

void	fib_processor::dataforDataService (const std::string &s,
	                                   packetdata *d,
	                                   int16_t compnr) {
int16_t	j;
serviceId *selectedService;


	d	-> defined	= false;	// always a decent default
	fibLocker. lock ();
	selectedService = findServiceId (s);
	if (selectedService == nullptr) {
	   fibLocker. unlock ();
	   return;
	}

	for (j = 0; j < 64; j ++) {
	   int16_t subchId;
	   if ((!ServiceComps [j]. inUse) ||
	                        (ServiceComps [j]. TMid != 03)) 
	      continue;

	   if (ServiceComps [j]. componentNr != compnr)
	      continue;

	   if (selectedService != ServiceComps [j]. service)
	      continue;

	   subchId		= ServiceComps [j]. subchannelId;
	   d	-> subchId	= subchId;
	   d	-> startAddr	= subChannels [subchId]. StartAddr;
	   d	-> shortForm	= subChannels [subchId]. shortForm;
	   d	-> protLevel	= subChannels [subchId]. protLevel;
	   d	-> length	= subChannels [subchId]. Length;
	   d	-> bitRate	= subChannels [subchId]. BitRate;
	   d	-> FEC_scheme	= subChannels [subchId]. FEC_scheme;
	   d	-> DSCTy	= ServiceComps [j]. DSCTy;
	   d	-> DGflag	= ServiceComps [j]. DGflag;
	   d	-> packetAddress = ServiceComps [j]. packetAddress;
	   d	-> appType	= ServiceComps [j]. appType;
	   d	-> defined	= true;
	   break;
	}
	fibLocker. unlock ();
}

void	fib_processor::dataforAudioService (const std::string &s,
	                                                 audiodata *d) {
	dataforAudioService (s, d, 0);
}

void	fib_processor::dataforAudioService (const std::string &s,
	                                    audiodata *d, int16_t compnr) {
int16_t	j;
serviceId *selectedService;

	d -> defined	= false;
	fibLocker. lock ();
	selectedService	= findServiceId (s);
	if (selectedService == nullptr) {
	   fibLocker. unlock ();
	   return;
	}

//	first we locate the serviceId
	for (j = 0; j < 64; j ++) {
	   int16_t subchId;
	   if ((!ServiceComps [j]. inUse) ||
	                        (ServiceComps [j]. TMid != 00))
	      continue;

	   if (ServiceComps [j]. componentNr != compnr)
	      continue;

	   if (selectedService != ServiceComps [j]. service)
	      continue;

	   subchId		= ServiceComps [j]. subchannelId;
	   d	-> subchId	= subchId;
	   d	-> startAddr	= subChannels [subchId]. StartAddr;
	   d	-> shortForm	= subChannels [subchId]. shortForm;
	   d	-> protLevel	= subChannels [subchId]. protLevel;
	   d	-> length	= subChannels [subchId]. Length;
	   d	-> bitRate	= subChannels [subchId]. BitRate;
	   d	-> ASCTy	= ServiceComps [j]. ASCTy;
	   d	-> language	= selectedService -> language;
	   d	-> programType	= selectedService -> programType;
	   d	-> defined	= true;
	   break;
	}
	fibLocker. unlock ();
}
//
//	and now for the would-be signals
//	Note that the main program may decide to execute calls
//	in the fib structures, so release the lock
void	fib_processor::addtoEnsemble	(const std::string &s, int32_t SId) {
	fibLocker. unlock ();
	if (theParameters -> programname_Handler != nullptr)
	   theParameters -> programname_Handler (s. c_str (), SId, userData);
	fibLocker. lock ();
}

void	fib_processor::nameofEnsemble  (int id, const std::string &s) {
	fibLocker. unlock ();
	if (theParameters -> ensemblename_Handler != nullptr)
	   theParameters -> ensemblename_Handler (s. c_str (), id, userData);
	fibLocker. lock ();
	isSynced	= true;
}

void	fib_processor::changeinConfiguration () {
}

bool	fib_processor::syncReached	() {
	return isSynced;
}

void	fib_processor::reset		() {
	dateFlag		= false;
	clearEnsemble	();
}

int32_t		fib_processor::get_CIFcount (void) {
	return CIFcount. load ();
}

