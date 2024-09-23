#
/*
 *    Copyright (C) 2015 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB-library
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

#include	"tdc-datahandler.h"

	tdc_dataHandler::tdc_dataHandler (int16_t appType,
	                                  bytesOut_t bytesOut,
	                                  void	*ctx) {
	this	-> bytesOut	= bytesOut;
	this	-> ctx		= ctx;
}

	tdc_dataHandler::~tdc_dataHandler (void) {
}

// CRC calculation
uint16_t calc_crc_bitts(uint8_t *data, uint32_t size) {
    uint16_t crc = 0xFFFF; // Initial value
    const ushort generator = 0x1021; /* divisor is 16bit */

    for (size_t byteIndex = 0; byteIndex < size; byteIndex++) {
          crc ^= (ushort(data[byteIndex]<<15));
          if ( crc & 0x8000 ) { crc = (ushort((crc<<1) ^ generator));
          } else { crc <<= 1; }
    }
    return ~crc;
}

/* General information regarding the data format can be found 
   a) EN 300 401 - V2.1.1 - Radio Broadcasting Systems; Digital Audio Broadcasting (DAB) to mobile, portable and fixed receivers
   b) https://www.imperial.ac.uk/media/imperial-college/research-centres-and-groups/centre-for-transport-studies/seminars/2009/TPEG---an-introduction-to-the-growing-family-of-standards-and-specifications-for-Traffic-and-Travel-Information-services.pdf
   c) DIN CEN ISO/TS 18234-2:2014-07 Intelligent transport systems -- Traffic and travel information (TTI) via transport protocol experts group, generation 2 (TPEG2) -- Part 2: UML model for TPEG2 UML
*/
/* ETSI TS 101 756 table 16 -> reference 6.3.6 of ETSI EN 300 401
                            User App Type = 4 -> ETSI TS 103 551 [8] -> TPEG
                            https://www.etsi.org/deliver/etsi_ts/103500_103599/103551/01.01.01_60/ts_103551v010101p.pdf
                            The MSC data groups shall be transported using the TDC in a packet mode service component with
                            data groups, as specified in ETSI TS 101 759 [3], clause 4.1.2.

*/
/* TPEG end sequenz  ff0f|0006|579e|00|01 41 0f |f7 |a8d0 <-- indicator for TPEG?
                     SYNC|LEN |CRC |FT|SID-A/B/C|ENC|???? 
      Sync Word:     dient dem Decoder zur Erkennung einer neuen Nachricht.
                     Dieses Sync Word hat immer den Wert FF0F hex.
      Field Length:  Gesamtlänge des Services Frames in Bytes. Ein Service Frame kann somit
                     nicht größer als 65535 Bytes sein.
      Frame Type:    sorgt für die weiter oben besprochene Unterscheidung zwischen
                     „Stream directory information“ und „Conventional service frame data“.
      Header CRC:    Prüfsumme um die Korrektheit der Headerdaten sicherzustellen. Diese Summe wird anhand
                     der Felder Sync Word, Field Length, Frame Type und der ersten 11 Byte des Service Frames berechnet.
                     Nähere Informationen zu dieser Berechnung finden sich in [TPEG2].
      Service Frame: enthält die Nutzdaten (evtl. in verschlüsselter Form) sowie die Service Identifier.
                     Über die Service Identifier (SID) kann ein Contentprovider eindeutig identifiziert werden.
                     Das Service Frame wird wiederum in folgende Bestandteile unterteilt:
      SID-A bis C:   ergeben zusammen eine eindeutige Identifikationsnummer eines Service Providers,
                     vergleichbar mit einer IP-Adresse, z. B. 133.168.123.
      Encryption-
      Indikator:     spezifiziert anhand eines Wertes zwischen 0 und 255 eine Verschlüsselungsmethode.
                     Die Werte 0 bis 127 sind dabei für standardisierte Methoden vorbehalten.
                     128–255 sind für die freie Verwendung durch einen Service Provider vorgesehen.
                     Die Verschlüsselung kann z. B. genutzt werden, um kostenpflichtige Dienste zu entwickeln.
                     Auch wäre die Verwendung verschlüsselter Nachrichten evtl. bei sicherheitskritischen Anwendungen,
                     wie z. B. Polizei- oder Militärfunk nutzbar.
      Component
      Multiplex:     Dieser evtl. verschlüsselte Datenbereich enthält dann die eigentlichen TPEG-Nachrichten,
                     Solange die Maximalgröße von 65531 Bytes nicht überschritten wird, kann dieser Bereich
                     mehrere Nachrichten aufnehmen. Die Kodierung dieser Daten ist der Spezifikation zu entnehmen.                
*/
void	tdc_dataHandler::add_mscDatagroup (std::vector<uint8_t> m) {
int32_t offset  = 0;
uint8_t *data   = (uint8_t *)(m. data ());
int32_t size    = m. size ();
int16_t i;
   
   // Get a brief overview of the input data
   fprintf(stderr,"DBG::tdc_dataHandler[%d]:", size/8);

   // Add MSC Data Group header processing
   uint8_t  extflg  = getBits (data,  0, 1);  // Extension Field Present Flag (see ETSI EN 300 401 - MSC data group header)
   uint8_t  crcflg  = getBits (data,  1, 1);  // CRC presence flag
   uint8_t  segfld  = getBits (data,  2, 1);  // Segment field presence flag
   uint8_t  uacfld  = getBits (data,  3, 1);  // User Access field precence
   uint8_t  dgtype  = getBits (data,  4, 4);  // Data group type
   uint8_t  cntidx  = getBits (data,  8, 4);  // Continuity index
   uint8_t  repidx  = getBits (data, 12, 4);  // Repetition index
   uint16_t extfld  = (extflg==1)?getBits (data, 16, 16):0;           // Extension field
   uint8_t  lastflg = (segfld==1)?getBits (data, 16+extfld*16,  1):0; // Last segment flag
   uint8_t  segidx  = (segfld==1)?getBits (data, 17+extfld*16,  7):0; // Segment number 
   uint8_t  rfa     = (segfld==1)?getBits (data, 24+extfld*16,  3):0; // Reserved for future use   
   uint8_t  tidflg  = (segfld==1)?getBits (data, 27+extfld*16,  1):0; // Transport Identifier flag
   uint8_t  tidlen  = (segfld==1)?getBits (data, 28+extfld*16,  4):0; // Field length of Transport Identifier and End User Address
   uint16_t tid     = (segfld==1)?getBits (data, 32+extfld*16, 16):0; // Transport Identifier
   uint8_t  dfoff   = 2 * 8 * (extflg + segfld);                      // Data field offset
   dfoff += uacfld * (tidlen+1) *8 ; // inlcude User Access Field, if present
   fprintf(stderr, "MSCDG: ext=%d crc=%d seg=%d uac=%d dgt=%d cnt=%d rep=%d last=%d seg=%d dfoff=%d crc=", extfld, crcflg, segfld, uacfld, dgtype, cntidx, repidx,lastflg,segidx,dfoff);
   
   // Check CRC of data group
   uint16_t dgcrc=0, chkcrc=1;
   if (crcflg==1) {
      dgcrc  = getBits (data, size-16, 16);
      chkcrc = calc_crc_bitts(data, size-16);
      fprintf(stderr,"%s ",dgcrc==chkcrc?"✓":"X");
   } else { fprintf(stderr,"- "); }

   // we maintain offsets in bits, the "m" array has one bit per byte
   while (offset < size) {
      while (offset + 16 < size) {
         if (getBits (data, offset, 16) == 0xFF0F) {
            break;
         }
         else
            offset += 8;
         }
         if (offset + 16 >= size) { 
            // NO syncword found! -> Output anyway, if CRC of data group is OK
            if (crcflg==1 && dgcrc==chkcrc) { 
               uint16_t length = size/8-((crcflg==1)?4:2)-dfoff/8;
               fprintf(stderr,"RAW off=%d len=%d\n",dfoff,length); // (size-(crcflg==1)?32:16));
               #ifdef _MSC_VER
                  uint8_t *buffer = (uint8_t *)_alloca(length);
               #else
                  uint8_t buffer[length];
               #endif
               if (bytesOut != nullptr)
                  bytesOut (buffer, length, 2, ctx);
            }
            return;
         }

// we have a syncword
//	uint16_t syncword    = getBits (data, offset,      16);
	uint16_t length      = getBits (data, offset + 16, 16);
//	uint16_t crc         = getBits (data, offset + 32, 16); 

   uint8_t frametypeIndicator = getBits (data, offset + 6 * 8,  8);
   if ((length < 0) || (length >= (size - offset) / 8))
      return;           // garbage

   uint8_t sida = getBits (data, offset + 7 * 8, 8);
   uint8_t sidb = getBits (data, offset + 8 * 8, 8);
   uint8_t sidc = getBits (data, offset + 9 * 8, 8);
   fprintf(stderr,"TPEG Frame%d SID=%d.%d.%d ",frametypeIndicator,sida,sidb,sidc);
   
   if (length >= 11) {
      // OK, prepare to check the crc
      uint8_t checkVector [18];
      
      // first the syncword and the length
      for (i = 0; i < 4; i ++) {
         checkVector [i] = getBits (data, offset + i * 8, 8);
      }

      // we skip the crc in the incoming data and take the frametype
      checkVector [4] = getBits (data, offset + 6 * 8, 8);
   
      int size = length < 11 ? length : 11;
      for (i = 0; i < size; i ++) {
         checkVector [5 + i]     = getBits (data,  offset + 7 * 8 + i * 8, 8);
      }
      checkVector [5 + size]     = getBits (data, offset + 4 * 8, 8); 
      checkVector [5 + size + 1] = getBits (data, offset + 5 * 8, 8); 
            
      if (check_crc_bytes (checkVector, 5 + size ) == 0) { 
         fprintf (stderr, "crc=X");
         return;
      } else { fprintf (stderr, "crc=✓"); }
   } fprintf(stderr,"\n");

   fprintf(stderr,"DBG::tdc_dataHandler::FrameType=%d off=%d len=%d\n",frametypeIndicator,offset+7*8,length);
   if (frametypeIndicator == 0)
      offset = handleFrame_type_0 (data, offset + 7 * 8, length);
   else
   if (frametypeIndicator == 1)
      offset = handleFrame_type_1 (data, offset + 7 * 8, length);
   else
      return;   // failure
   //	fprintf (stderr, "offset is now %d\n", offset);
   if (offset < 0)
      return;
      }
}

int32_t tdc_dataHandler::handleFrame_type_0 (uint8_t *data,
                                            int32_t offset, int32_t length) {
int16_t i;
int16_t noS     = getBits (data, offset, 8);
#ifdef _MSC_VER
uint8_t	*buffer = (uint8_t *)_alloca(length);
#else
uint8_t	buffer [length];
#endif

	(void)noS;
	for (i = 0; i < length; i ++)
	   buffer [i] = getBits (data, offset + i * 8, 8);

	if (bytesOut != nullptr)
	   bytesOut (buffer, length, 0, ctx);
	return offset + length * 8;
}

int32_t tdc_dataHandler::handleFrame_type_1 (uint8_t *data,
                                             int32_t offset, int32_t length) {
int16_t i;
#ifdef _MSC_VER
uint8_t	*buffer = (uint8_t *)_alloca(length);
#else
uint8_t	buffer [length];
#endif

	for (i = 0; i < length; i ++)
	   buffer [i] = getBits (data, offset + i * 8, 8);

	if (bytesOut != nullptr)
	   bytesOut (buffer, length, 1, ctx);
        return offset + length * 8;
}

//      The component header CRC is two bytes long,
//      and based on the ITU-T polynomial x^16 + x*12 + x^5 + 1.
//      The component header CRC is calculated from the service component
//      identifier, the field length and the first 13 bytes of the
//      component data. In the case of component data shorter
//      than 13 bytes, the component identifier, the field
//      length and all component data shall be taken into account.
bool    tdc_dataHandler::serviceComponentFrameheaderCRC (uint8_t *data,
                                                         int16_t offset,
                                                         int16_t maxL) {
uint8_t testVector [18];
int16_t i;
int16_t length  = getBits (data, offset + 8, 16);
int16_t size    = length < 13 ? length : 13;
uint16_t        crc;
        if (length < 0)
           return false;                // assumed garbage
        crc     = getBits (data, offset + 24, 16);      // the crc
        testVector [0]  = getBits (data, offset + 0,  8);
        testVector [1]  = getBits (data, offset + 8,  8);
        testVector [2]  = getBits (data, offset + 16, 8);
        for (i = 0; i < size; i ++)
           testVector [3 + i] = getBits (data, offset + 40 + i * 8, 8);
	testVector [3 + size    ] = getBits (data, offset + 24, 8);
	testVector [3 + size + 1] = getBits (data, offset + 32, 8);
	(void)crc;
        return check_crc_bytes (testVector, 5 + size) == 0;
}


