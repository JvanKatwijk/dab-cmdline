/*
 *    Copyright (C) 2015
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    Added Reed-Solomon-Decoder for Packet Data in 2024
 *    by Stefan Juhl (stefanjuhl75@gmail.com)
 *
 *    This file is part of DAB library
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
 */

#include	"dab-constants.h"
#include	"data-processor.h"
#include	"virtual-datahandler.h"
#include	"mot-handler.h"
#include    "tdc-datahandler.h"

// 11-bit from HandleFIG0Extension13, see ETSI TS 101 756 table 16
// AppType -> https://www.etsi.org/deliver/etsi_ts/101700_101799/101756/02.02.01_60/ts_101756v020201p.pdf
/*char *getUserApplicationType (int16_t appType) {
	char *buffer = (char *)malloc(30);
        switch (appType) {
           case 1:     return "Dynamic labels (X-PAD only)";
           case 2:     return "MOT Slide Show";		// ETSI TS 101 499
           case 3:     return "MOT Broadcast Web Site";
           case 4:     return "TPEG";			// ETSI TS 103 551
           case 5:     return "DGPS";
           case 6:     return "TMC";
           case 7:     return "SPI, was EPG";		// ETSI TS 102 818
           case 8:     return "DAB Java";
           case 9:     return "DMB";			// ETSI TS 102 428
           case 0x00a: return "IPDC services";
           case 0x00b: return "Voice applications";
           case 0x00c: return "Middleware";
           case 0x00d: return "Filecasting";		// ETSI TS 102 979
           case 0x44a: return "Journaline";
           default:
	       sprintf(buffer, "(0x%04x)", appType);;
	       return buffer;
        }
        return "";
} */

// Arrays for Reed-Solomon Data Field and Packet CRC Flag
uint8_t rsf[12][16];	    // Data Field
//uint8_t pcf[94];		    // Packet CRC Flag
bool chkRS = false;          // Check Reed-Solomon Decoder
uint8_t maxCorruptBytes=8;	// Packet CRC Flag
uint64_t packets=0;	        // Global Packet Counter
uint64_t skipped=0;	        // Global Packet Counter
uint8_t cntPkt=0;           // Packet Counter
uint64_t PktSum=0;          // Sum of Packet Lengths

// CRC calculation
uint16_t calc_crc_bits(uint8_t *data, uint32_t size) {
    uint16_t crc = 0xFFFF; // Initial value
    const ushort generator = 0x1021; /* divisor is 16bit */

    for (size_t byteIndex = 0; byteIndex < size; byteIndex++) {
          crc ^= (ushort(data[byteIndex]<<15));
          if ( crc & 0x8000 ) { crc = (ushort((crc<<1) ^ generator));
          } else { crc <<= 1; }
    }
    return ~crc;
}

#include <vector>
#include <cstdint>
/*  The packet CRC shall be a 16-bit CRC word calculated on the packet header and the packet data field. It shall be
    generated according to the procedure defined in annex E. The generation shall be based on the polynomial
    Recommendation ITU-T X.25 / Annex E of ETSI EN 300 401 V2.1.1 (2017-01) */
uint16_t calc_crc_bytes(const std::vector<uint8_t>& outVector, size_t start, int16_t len) {
    int i, j;
    uint16_t accumulator = 0xFFFF;
    uint16_t genpoly = 0x1021;

    for (i = 0; i < len; i++) {
        int16_t data = outVector[start + i] << 8;
        for (j = 8; j > 0; j--) {
            if ((data ^ accumulator) & 0x8000)
                accumulator = ((accumulator << 1) ^ genpoly) & 0xFFFF;
            else
                accumulator = (accumulator << 1) & 0xFFFF;
            data = (data << 1) & 0xFFFF;
        }
    }

    // CRC-Wert berechnen
    uint16_t crc = ~accumulator & 0xFFFF;
    return crc;
}

void printArrayAsHex(uint8_t* array, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        // Print each byte as a two-digit hexadecimal value
        fprintf(stderr,"%02x", array[i]);

        // Optionally: print a new line after every 16 values for readability
        if ((i + 1) % 24 == 0) {
            fprintf(stderr,"\n     ");
        }
    }
    fprintf(stderr,"\n");
}

#include <cctype> // Für isprint

// Hilfsfunktion zum Ausgeben eines Arrays als ASCII-Zeichen
void printArrayAsASCII(const uint8_t* data, size_t length) {
    for (size_t i = 3; i < (length-5); i++) {
        if (isprint(data[i])) {
            fprintf(stderr, "%c", data[i]);
        } else {
            fprintf(stderr, "_");
        }
    }
    fprintf(stderr, "\n");
}

//	class dataProcessor
//	The main function of this class is to assemble the
//	MSCdatagroups and dispatch to the appropriate handler
//
//	fragmentsize == Length * CUSize
	dataProcessor::dataProcessor	(int16_t	bitRate,
	                                 packetdata	*pd,
	                                 API_struct	*p,
	                                 void	        *ctx):
	                                 my_rsDecoder (8, 0435, 0, 1, 16) {
/* 	NOTE Reference http://web.eecs.utk.edu/~jplank/plank/papers/CS-05-570.html
	NOTE mp4Processor:               my_rsDecoder (8, 0435, 0, 1, 10),

	reedSolomon (uint16_t symsize   = 8,	// symbol size, bits (1-8)
		     	 uint16_t gfpoly    = 0435, // Field generator polynomial coefficients
                 uint16_t fcr       = 0,	// first root of RS code generator polynomial, index form, 0
                 uint16_t prim      = 1,	// primitive element to generate polynomial roots 2->19
                 uint16_t nroots    = 10);	// RS code generator polynomial degree (number of roots) 16->255

	NOTE The primitive polynomials used for the Galois Fields are specified in octal below:
	     Octal 435 -> w= 8
	                  w= 9: 01021
	                  w=10: 02011

	The code used is the Reed-Solomon RS (204,188, t = 8) shortened code (see note 2), derived
	from the original systematic RS (255,239, t = 8) code.

	The Reed-Solomon codeword has length 204 bytes, dimension 188 bytes and allows up to 8 random
	erroneous bytes in a received word of 204 bytes to be corrected.

	The shortened Reed-Solomon code may be implemented by adding 51 bytes, all set to zero,
	before the information bytes (i.e. one row of the Application Data Table) at the input of an
	RS (255,239, t = 8) encoder. After the RS coding procedure these null bytes shall be discarded,
	leading to a RS codeword of N = 204 byte	*/

	this	-> bitRate		 = pd -> bitRate;
	this	-> DSCTy		 = pd -> DSCTy;
	this	-> appType		 = pd -> appType;
	this	-> packetAddress = pd -> packetAddress;
	this	-> DGflag		 = pd -> DGflag;
	this	-> FEC_scheme	 = pd -> FEC_scheme;
    this    -> pdlen         = pd -> length * 4;
	this	-> bytesOut		 = p  -> bytesOut_Handler;
	this    -> mscQuality    = p  -> program_quality_Handler; // added S. Juhl
	this	-> ctx			 = ctx;
        RSDims               = 12; 	 // mp4: 8 -> 12 rows

        frameBytes.resize(0);
        outVector.resize (RSDims * 188); 	// (RSDims * 110); // 2.256 Bytes Application Data Table

        blockFillIndex  = 0;
        blocksInBuffer  = 0;
        curMSC 		    = 16;
        curPI		    = 255;

        frameCount      = 0;
        frameErrors     = 0;
        rsErrors        = 0;
        crcErrors	    = 0;

        frame_quality   = 0;
        rs_quality      = 0;

	fprintf (stderr, "** DBG: dataProcessor: appType=%d FEC=%d DSCTy=%d len=%d(", pd -> appType, FEC_scheme, pd -> DSCTy, pd->length);
	switch (DSCTy) {
	   default:
	      my_dataHandler	= new virtual_dataHandler ();
	      break;

	   case 5:			// do know yet
	      fprintf(stderr,"DBG: TDC data handler active)\n");
	      my_dataHandler	= new tdc_dataHandler (appType, bytesOut, ctx);
	      break;

	   case 60:
	      my_dataHandler	= new motHandler (p -> motdata_Handler, ctx);
	      break;
	}
}

	dataProcessor::~dataProcessor	(void) {
	delete		my_dataHandler;
}


void	dataProcessor::addtoFrame (uint8_t *outV) {
//	There is - obviously - some exception, that is
//	when the DG flag is on and there are no datagroups for DSCTy5
	/* MSC Quality report // added S. Juhl */
    /* uint8_t payload = getBits_7(outV, 16); // Useful data field length
    fprintf (stderr, "DBG: dataProcessor::addtoFrame[%2d] payload=%3d ",dataProcessor::pdlen,payload);
    for (uint8_t j = 0; j < 24; j++) {
        fprintf(stderr, "%d", (outV[j] & 0x01));
        if (j == 1 || j == 3 || j == 5 || j == 15 || j== 16 ) {
            fprintf(stderr, "|");
        }
    } fprintf(stderr, "\n"); */

	if (++frameCount >= 100) {
			frameCount    = 0;
			frame_quality = 1 * (100 - frameErrors);
			rs_quality    = 100 * (1 - rsErrors/12);
			if (mscQuality != nullptr)
				mscQuality (frame_quality, rs_quality, 1*(100-crcErrors), ctx);
			frameErrors = crcErrors = rsErrors = 0;
	}

	if ((this -> DSCTy == 5) &&
	    (this -> DGflag)) {			// no datagroups
	      handleTDCAsyncstream (outV, 24 * bitRate);
	} else {
        if (outV == nullptr) {
            fprintf(stderr, "AddressSanitizer: Null pointer in addtoFrame\n");
            exit(22);
        }
	      handlePackets (outV, 24 * bitRate);	// Data group packets
	}
}

/* static inline
void setBits_2(uint8_t *d, int32_t offset, uint16_t value) {
    d[offset] = (value >> 1) & 0xFF;         // Set the higher 7 bits of the value to d[offset]
    d[offset + 1] = (d[offset + 1] & ~0x01) | (value & 0x01);  // Set the lowest bit of the value to d[offset + 1]
} */

// Helper function to process RS data (8 Bit to 1 Byte)
uint8_t dataProcessor::processRSData(uint8_t *data, uint16_t i) { 
    uint8_t temp = 0;
    for (uint8_t j = 0; j < 8; j++) {
        temp = (temp << 1) | (data[(i + 2) * 8 + j] & 0x01);
    }
    return temp;
}

uint8_t AppDTCnt=0;
//
// While for a full mix data and audio there will be a single packet in a
// data compartment, for an empty mix, there may be many more
/*	VERIFIED!   TPEG		appType=4	 FEC=1 344 / ETSI TS 103 551
	TBD         TPEG_MM		appType=4	 FEC=0 152
	VERIFIED!   PPP-RTK-AdV	appType=1500 FEC=1 192
*/
void dataProcessor::handlePackets(uint8_t *data, int16_t length) { //  192 or 384 bytes
    // Calculate packetLength and address
    uint8_t  packetLength = (getBits_2(data, 0) + 1) * 24;  // 24/48/72/96 bytes
    uint16_t address      =  getBits(data, 6, 10);          // 0 -> only for padding
    
    if (packetLength > dataProcessor::pdlen) packetLength = dataProcessor::pdlen;
    
    // Check packet for FEC address
    if (address == 1022) {
        handleRSdata(data);                                   // Process Reed-Solomon data field
        if (dataProcessor::pdlen == packetLength) return;
        // If present, process further FEC packets
        for (uint8_t cp = 1; cp < (dataProcessor::pdlen/packetLength); cp++) {
            uint8_t *dataz = data+cp*packetLength*8;        // define a new pointer to the position of 2nd FEC packet
            address = getBits(dataz, 6, 10);                // Get address of the 2nd part
            if (address == 1022) { // another one?
                handleRSdata(dataz);
                return;
            } 
            data+=(address != 1022)?24*8:0;                 // skip the FEC packet
        } 
    } else if (packetLength != dataProcessor::pdlen) {
        uint8_t *dataz = data+1*packetLength*8;             // define a new pointer for 2nd part of the packet
        address = getBits(dataz, 6, 10);                    // Get address of the 2nd part
        handleAppData(data, packetLength-5);                   // Has to be done, BEFORE processing RS data bytes!
        if (address == 1022) { 
            handleRSdata(dataz);
            return;
        } 
        data+=(address != 1022)?24*8:0;                     // skip the first part of the packet        
    }
       
    handleAppData(data, packetLength-5);
    return; // skip the rest of the code //
    /*=====//===========================//=====*/
    
    /* orginal code
    while (true) {
	   int16_t pLength = (getBits_2 (data, 0) + 1) * 24 * 8;
	   if (length < pLength)	// be on the safe side
	      return;
	   handlePacket (data);
	   length -= pLength;
	   if (length < 2)
	      return;
	   data	= &(data [pLength]);
	}*/


    uint8_t  RSF_counter;
    packets++;

    uint8_t usefulLength; // = getBits_7(data, 17); // Useful data field length **
    uint16_t pcrc; //        = getBits(data, (packetLength - 2) * 8, 16);
    uint16_t ccrc; //        = calc_crc_bits(data, (packetLength - 2) * 8);
    
// Let's go for the application data
    usefulLength = getBits_7(data, 17); // Useful data field length **
    pcrc         = getBits(data, (packetLength - 2) * 8, 16);
    ccrc         = calc_crc_bits(data, (packetLength - 2) * 8);
    uint32_t zeroc       = getBits(data, (packetLength - 4) * 8, 16) +
                          (getBits(data, (packetLength - 6) * 8, 16) << 16);
    

    uint8_t FirstFEC     = getBits_1(data, 4);
    
    // adjust packet length bits, if necessary
    if (address != 1022 && packetLength != dataProcessor::pdlen) {
        //packetLength = dataProcessor::pdlen;

        //setBits_2(data, 0, dataProcessor::pdlen / 24 - 1);  // adjust packet length bits
        //skipped++;
    } else if (address == 1022) {
        RSF_counter = getBits_4(data, 2);                       // RS packets counter
    }
    
    // Start processing the packet
    //uint8_t usefulLength = (address == 1022)?22:getBits_7(data, 17); // Useful data field length **
    //uint16_t pcrc        = getBits(data, (packetLength - 2) * 8, 16);
    //uint16_t ccrc        = calc_crc_bits(data, (packetLength - 2) * 8);
    //uint32_t zeroc       = getBits(data, (packetLength - 4) * 8, 16) +
    //                      (getBits(data, (packetLength - 6) * 8, 16) << 16);
    //uint8_t FirstFEC     = getBits_1(data, 4);

    /* Handle FEC scheme
       NOTE: The FEC scheme protects all packets in the sub-channel irrespective of their packet address. */
    if (FEC_scheme == 1 && ((pcrc == 0 && zeroc == 0) || address == 1022)) {
        if (RSF_counter >= 8) AppDTCnt=0; 
        handleRSdata(data); // Process Reed-Solomon data field
    } else {    // Application Data Table Processing
        AppDTCnt++;
        if (FirstFEC == 1) {
            AppDTCnt=1;
            fprintf(stderr, "DBG::handlePacket:FIRST PACKET----------------------------------------\n");  
            uint8_t extf=getBits_1(data, 24); // Extension flag
            uint8_t crcf=getBits_1(data, 25); // CRC at end of MSC data group?
            uint8_t segf=getBits_1(data, 26); // Segment field present?
            uint8_t uaf=getBits_1(data, 27);  // User access field present?
            uint8_t dgt=getBits_4(data, 28);  // Data group type
            uint8_t cnti=getBits_4(data, 32); // Continuity index
            uint8_t repi=getBits_4(data, 36); // Repetition index
            fprintf(stderr, "DBG::handlePacket:MSC Data Group Header Ext:%d CRC:%d Seg:%d UAF:%d DGT:%d CntIdx:%d RepIdx:%d",extf,crcf,segf,uaf,dgt,cnti,repi);
            if (extf ==1) {
                fprintf(stderr, " ExtFld=0x%02x%02x",getBits_8(data, 40),getBits_8(data, 48));
            } fprintf(stderr, "\n");
            if (segf == 1) {
                uint8_t shlf=getBits_1(data, 40+(extf==1?16:0)); // Session Header Last Flag
                uint8_t segh=getBits_7(data, 41+(extf==1?16:0));  // Session Header Flag High-Byte
                uint8_t segl=getBits_8(data, 48+(extf==1?16:0));  // Session Header Flag Low-Byte
                fprintf(stderr, "DBG::handlePacket:SegmentField: Last:%d SegNo:%02x%02x\n",shlf,segh,segl);
            }
            if (uaf ==1 ) {
                uint8_t rfa=getBits_3(data, 40+(extf==1?16:0)+(segf==1?16:0)); // Reserved for future addition
                uint8_t tidf=getBits_1(data, 43+(extf==1?16:0)+(segf==1?16:0)); // Transport ID present?
                uint8_t uali=getBits_4(data, 44+(extf==1?16:0)+(segf==1?16:0)); // Length indicator
                uint8_t tidl,tidh;
                if (tidf == 1) {
                    tidh=getBits_8(data, 48+(extf==1?16:0)+(segf==1?16:0));  // Transport ID High-Byte
                    tidl=getBits_8(data, 56+(extf==1?16:0)+(segf==1?16:0));  // Transport ID Low-Byte
                } else { tidh=tidl=0; }
                fprintf(stderr, "DBG::handlePacket:UserAccessField: RFA:%d TID:%02x%02x Len:%d\n",rfa,tidh,tidl,uali);
            }
        }
        if (getBits_1(data, 5) == 1) fprintf(stderr, "DBG::handlePacket:LAST PACKET of %d bytes payload, framesBytes has %ld bytes ------------------------------------\n",AppDTCnt*(packetLength-5),frameBytes.size());  
        if (getBits_2(data, 4) == 3) { 
            fprintf(stderr, "\n!!!!!!!!!!!!!!!!\nDBG::handlePacket:SINGLE PACKET of %d bytes payload ----------------------------------------\n",AppDTCnt*(packetLength-5));  
            // fprintf(stderr, "DBG::handlePacket:Hexdump: ");
            for (int8_t i = 0; i < packetLength-2; i++) {
                uint8_t temp = 0;
                temp = processRSData(data, i); // , temp);  // Refactored helper function
                fprintf(stderr, "%02x", temp);
            } fprintf(stderr, "|"); 
            for (int8_t i = 0; i < packetLength-2; i++) {
                if (isprint(data[i])) {
                    fprintf(stderr, "%c", data[i]);
                } else {
                    fprintf(stderr, "_");
                }
            }  fprintf(stderr, "\n"); 
        }
        fprintf(stderr, "DBG::handlePacket: Adr=%4d, PLen=%d, DLen=%d, ULen=%d ", address, dataProcessor::pdlen, packetLength, usefulLength);
    }    
    
}


// Handle sequence of FEC packets and RS decoding
void dataProcessor::handleRSdata(uint8_t *data) {
    //const uint16_t RS_BLOCK_SIZE = 2448;  // Total RS data size
    const uint16_t RS_PACKET_SIZE = 22;   // Number of bytes in each RS packet
    //const uint16_t FRAME_SIZE = 188 * 12; // Total frame size
    //const uint8_t COMPLETE_BLOCKS = 8;    // Number of blocks for complete data
    const uint16_t MAX_ROWS = 12;         // Number of rows in RS matrix
    const bool debug = false;
    
    int16_t i;
    uint16_t rs_dt_pos = 0;
    uint8_t col, row, temp = 0;
    uint8_t Counter; // = getBits_4(data, 2);
    

    // Process Reed-Solomon data
    /* NOTE: The Reed-Solomon data field is a sequence of 8 RS packets, each containing 22 bytes of data.
             The RS packets are transmitted in sequence, with the first RS packet containing the first 22 bytes
             of the data field, the second RS packet containing the next 22 bytes, and so on.
             If the packet size of the packet data stream is more than 24 bytes in size, RS packets will be
             added consecutively to fill the RS data packet up to the required size.
    */
    //uint8_t hlf   = getBits_1(data, 5);
    //uint8_t DFcnt = dataProcessor::pdlen / 24;
    /* if (DFcnt > 0) {
        uint8_t cntpz = getBits_4(data,24*8+2); // Address of backpacked FEC packet
        fprintf(stderr, "DBF::handleRSdata:Counter1=#%2d / Counter2=0x%02x",Counter,cntpz); }*/
    for (uint8_t j=0; j < 1; j++ ) { // j < (DFcnt); j++) {
        Counter = getBits_4(data, j*24*8+2); // Counter 0..7
        if (true) { //(DFcnt > 0 && j != 0 && Counter != 8) {
            if (debug) fprintf(stderr, "DBG::handleRSdata:j=%d:Counter#%2d ",j,Counter);
            for (i = 0; i < RS_PACKET_SIZE; i++) { 
                temp = processRSData(data, i+24*j); // , temp);  // Refactored helper function 
                // Assign to rsf matrix
                rs_dt_pos = RS_PACKET_SIZE * Counter + i;
                row = rs_dt_pos / MAX_ROWS;
                col = rs_dt_pos % MAX_ROWS;
                if (rs_dt_pos < 192) {
                    rsf[col][row] = temp; // fprintf(stderr, " RS%2d:data[%3d] -> rsf[%02d][%02d]\n", i, rs_dt_pos, col, row);
                    if (debug) fprintf(stderr, "%02x", temp);
                } else if (debug) { fprintf(stderr, "%s%02x", (rs_dt_pos == 192)?"|":"",temp); }
            } if (debug && Counter < 0xff) fprintf(stderr, "\n");
        }
    }
        
    // Apply FEC when all parts are received
    if (!frameBytes.empty() && Counter >= 8) applyFEC(); 
    return;
}

#include <iostream>
#include <random>

// Simulate data corruption by randomly changing bytes in an array
void corruptData(uint8_t* array, size_t arraySize, size_t numCorruptBytes) {
    if (numCorruptBytes > arraySize) {
        std::cerr << "Fehler: Anzahl der zu korrumpierenden Bytes übersteigt die Arraygröße." << std::endl;
        return;
    }

    // Initalize random number generator
    std::random_device rd;
    std::mt19937 gen(rd());  // Mersenne-Twister-Engine für Zufallszahlen
    std::uniform_int_distribution<> byteIndexDistr(0, arraySize - 1);  // Zufällige Position im Array
    std::uniform_int_distribution<> byteValueDistr(0, 255);  // Zufälliger Bytewert

    // Randomly corrupt bytes
    for (size_t i = 0; i < numCorruptBytes; ++i) {
        size_t index = byteIndexDistr(gen);  // Wähle zufälligen Index im Array
        //uint8_t oldValue = array[index];     // Alter Wert für Debugging
        array[index] = byteValueDistr(gen);  // Setze zufälligen neuen Wert

        /* Debug-Ausgabe, falls gewünscht
        std::cout << "Byte an Position " << index << " geändert von "
                  << static_cast<int>(oldValue) << " zu "
                  << static_cast<int>(array[index]) << std::endl;*/
    }
}

#include <cstring> // Für memcmp
#include <cstdio>  // Für fprintf
// Reed-Solomon decoder verification
/* NOTE see -> https://scholarworks.calstate.edu/downloads/vh53wz89h
		(204, 188) REED-SOLOMON CODE ENCODER/DECODER DESIGN, SYNTHESIS, AND SIMULATION ...
		A graduate project submitted in partial fulfillment of the requirements
		For the degree of Master of Science in Electrical Engineering
		By Haoyi Zhang
		sample data table for dt[0]=1, dt[1]=2 .. dt[187]=188
		k=51 -> 0xc3e75ac28e7055ab3ff2fb9a015221de */
void dataProcessor::verifyRSdecoder(uint8_t* rsIn, uint8_t* rseIn, uint8_t* rsOut) { 
    if (memcmp(rseIn, rsOut, 188) == 0 && !chkRS) {
        fprintf(stderr,"applyFEC::verifyRSdecoder\n");
        /* fprintf(stderr,"FEC: ");
        printArrayAsHex(rsIn, 204); */
        corruptData(rsIn, 188, maxCorruptBytes); // Simulate data corruption
        /* fprintf(stderr,"COR: ");
        printArrayAsHex(rsIn, 188); */
        my_rsDecoder.dec(rsIn, rsOut, 51);
        uint8_t rsc = memcmp(rseIn, rsOut, 188);
        if (rsc == 0) {
            /* fprintf(stderr,"REC: ");
            printArrayAsHex(rsOut, 188); */
            fprintf(stderr,"-> SUCESSFULLY recovered %d corrupted bytes.\n", maxCorruptBytes);
            chkRS = true;
        } else {
            fprintf(stderr,"failed recovering %d corrupted bytes - retrying.\n", maxCorruptBytes);
            maxCorruptBytes--;
        }
    }
}

//  Apply reed solomon forward error correction
/*  (204, 188) Reed-Solomon code is a truncated version of RS(255, 239) code. It deletes the
	first 51 symbols of RS(255, 239) code since they are all zeros. Therefore, their design
	and realization methods are similar. RS(204, 188) code is defined on Galois field GF(2^8). */
void dataProcessor::applyFEC(void) {
    // Use dynamic memory allocation to avoid frequent stack allocation
    // Fill up Application Data Table up to 2256 bytes, tbd if necessary
    const bool debug = false;
    uint8_t* rsIn = new uint8_t[204];
    uint8_t* rsOut = new uint8_t[188];
    uint8_t* rseIn = new uint8_t[188];
    //uint8_t pcf[94];		    // Packet CRC Flag
    uint8_t rsc[RSDims];        // RS Result Checker

    uint16_t k;
    uint8_t rsek = 0, base = 0; // rse
    
    if (debug) fprintf(stderr, "DBG: applyFEC on frameBytes[%ld]\n",frameBytes.size());
    
    /* Print frameBytes for debugging */
    //fprintf(stderr, "FEC::frameBytes[%ld]\n", frameBytes.size());
    //fprintf(stderr, "----------------------------------------\n");
    //printArrayAsHex(frameBytes.data(), 48 ); // frameBytes.size());
    //printArrayAsASCII(frameBytes.data(), frameBytes.size());
    //fprintf(stderr, "----------------------------------------\n"); 

    // Helper function to fill rsIn and rseIn
    auto fillRsIn = [&](uint16_t j, uint16_t k) {
        uint32_t index = (base + j + k * RSDims) % (RSDims * 204);
        if (index < frameBytes.size()) {
            rsIn[k] = frameBytes[index];
            if (k < 188) { rseIn[k] = rsIn[k]; }
        } else { rsIn[k] = 0; }
    };

    // Apply forward error correction
    for (uint16_t j = 0; j < RSDims; j++) {
        if (debug) fprintf(stderr, "Row#%02d: ", j+1);
        for (k = 0; k < 204; k++) {
            fillRsIn(j, k);
            if (k > 187) {
                rsIn[k] = rsf[j][k - 188];  // Copy RS Data Field
                if (debug) fprintf(stderr, "%02x", rsf[j][k - 188]);
            } 
        }
        rsc[j] = ((my_rsDecoder.dec(rsIn, rsOut, 51) < 0) ? 1 : 0); // braces or rsek += rse; necessary
        if (debug) fprintf(stderr, " %s\n",rsc[j]?"X":"✓");
        rsek += rsc[j];
        // Copy the corrected data to outVector
        for (k = 0; k < 188; k++) {
            outVector[j + k * RSDims] = rsOut[k]; // Bypass Out[k];
        }
    } fprintf(stderr, "FEC::RSQ[<%d Err.Bytes] %.1f%% crc=%d\n",maxCorruptBytes ,100.0-100*rsek/12,crcErrors);
    
    // Post-FEC check for efficiency of RS Decoder
    verifyRSdecoder(rsIn, rseIn, rsOut);

    // Process packet stream
    // processPacketStream(); // Process the packet stream
    processOutVector(outVector);
    
    // Clean up and update error count
    frameBytes.resize(0);
    //series.resize(0); will be done in processOutVector
    rsErrors += rsek;

    // Free dynamically allocated memory
    delete[] rsIn;
    delete[] rsOut;
    delete[] rseIn;
    return;
}

#include <iostream>
#include <vector>
#include <algorithm> // Für std::reverse


// Funktion zum Extrahieren von Bits aus einem Byte-Array
uint32_t getBitss(const std::vector<uint8_t>& data, size_t& bitOffset, size_t numBits) {
    uint32_t result = 0;
    for (size_t i = 0; i < numBits; ++i) {
        size_t byteOffset = bitOffset / 8;
        size_t bitInByte = 7 - (bitOffset % 8);
        result <<= 1;
        result |= (data[byteOffset] >> bitInByte) & 1;
        ++bitOffset;
    }
    return result;
}

// convert Bytes back to bit vector
std::vector<uint8_t> bytes_to_bits(const std::vector<uint8_t>& bytes) {
    //fprintf(stderr, "bytes2bits: ");
    std::vector<uint8_t> bits;
    for (uint8_t byte : bytes) {
        //fprintf(stderr, "%02x", byte);
        for (int i = 7; i >= 0; --i) {
            bits.push_back((byte >> i) & 0x01);
        } 
    } //fprintf(stderr, "\n");
    return bits;
}

// Function to parse FEC processed packet, see section 5.3.2 of ETSI EN 300 401 V2.1.1 (2017-01)
// CRC calculation verified using CRC16-GENIBUS -> https://crccalc.com/
// Orginal code to hand over to add_MSCdatagroup?
void dataProcessor::processOutVector(const std::vector<uint8_t>& outVector) {
    /* uint16_t bo=0;
    fprintf(stderr,"--> RAW PACKET (%4ld Bytes) <-------------------\n",outVector.size());
    while (bo < outVector.size()) {
        fprintf(stderr,"%02x",outVector[bo]);
        bo++;
        if ((bo % dataProcessor::pdlen == 0) ) {
            fprintf(stderr,"\n");
        }
    }
    fprintf(stderr,"------------------------------------------------\n");*/
    
    const bool debug = false;
    uint16_t boi=0; 
    uint8_t  packet_length; 
    uint8_t  continuity_index;
    uint8_t  packet_number = 1;
    uint8_t  firstlast_flags;
    uint16_t packet_adrress;
    uint8_t  command_flag;
    uint8_t  useable_length;
    uint16_t packet_crc;
    uint16_t check_crc;
    uint8_t  crc_chk_len;

    while (boi < outVector.size()) {
        packet_length    = ((outVector[boi] >> 6) & 0x03) * 24 + 24;            //  2 bits
        continuity_index =  (outVector[boi] & 0x30) >> 4;                       //  2 bits
        firstlast_flags  =  (outVector[boi] & 0x0C) >> 2;                       //  2 bits
        packet_adrress   =  (outVector[boi] & 0x03) << 8 | outVector[boi + 1];  // 10 bits
        command_flag     =  (outVector[boi + 2] & 0x80) >> 7;                   //  1 bit
        useable_length   =  (outVector[boi + 2] & 0x7F);                        //  7 bits, <= 91
    
        // Data validity checks: packet length, useable data length, continuity index
        if (packet_length > dataProcessor::pdlen) packet_length = dataProcessor::pdlen;
        if (useable_length == 0 || useable_length > 91 || useable_length > dataProcessor::pdlen) useable_length = packet_length-5;

        // Calculate CRC check length and perform CRC extraktion and calculation
        crc_chk_len      =  packet_length<=dataProcessor::pdlen?packet_length-2:22;
        packet_crc       =  (outVector[boi + crc_chk_len ]) << 8 
                           | outVector[boi + crc_chk_len + 1];
        check_crc        =  calc_crc_bytes(outVector, boi, crc_chk_len); 
        
        if (check_crc != 0x604b) {  // don't care about zeroed packets ...
            if (debug) fprintf(stderr,"#%2d raw=%02x padr=%4d pl=%02d ul=%2d cl=%d ci=%d fl=%d cf=%d crc=%04x/%04x %s ",packet_number,outVector[boi],packet_adrress, packet_length,useable_length, crc_chk_len,  continuity_index,firstlast_flags,command_flag,packet_crc,check_crc,(packet_crc!=check_crc)?"X":"✓");
            // Check Sequence Continuity
            if (curPI > 4 || continuity_index == ((curPI + 1) % 4)) {
                curPI = continuity_index;
            } else {
                fprintf(stderr, "DBG::Continuity Error: %d -> %d\n", curPI, continuity_index);
                series.resize(0);
                curPI=0xff;
                return;
            }
            
            // Print the packet data
            for (uint8_t i = 0; i < useable_length; ++i) {
                if (debug) fprintf(stderr,"%02x",outVector[boi + 3 + i]);
            } if (debug) fprintf(stderr,"\n");
            
            // CRC Check
            std::vector<uint8_t> relevant_bytes(outVector.begin() + boi + 3, outVector.begin() + boi + 3 + useable_length);
            //std::vector<uint8_t> relevant_bytes(outVector.begin() + boi, outVector.begin() + boi + crc_chk_len + 2);
            std::vector<uint8_t> bits = bytes_to_bits(relevant_bytes);  // Konvertieren Sie die Bytes in Bits
            series.insert(series.end(), bits.begin(), bits.end());      // Fügen Sie die Bits zum series-Array hinzu

            switch(firstlast_flags) {
                case 0: // Middle packet
                    //fprintf(stderr, "Middle packet %ld\n",series.size());
                    break;
                case 2: // First packet
                    //fprintf(stderr, "First packet\n");
                    if (series.size() > 0) {
                        fprintf(stderr, "First packet series[%ld] ...\n",series.size());
                        // TBD, what if series is not empty?
                    }   
                    break;
                case 1: // Last packet
                case 3: // Single packet
                    if (debug) fprintf(stderr, "%s AppType=%d packet -> add_mscDG[%ld] ...\n",(firstlast_flags==1)?"Last":"Single",dataProcessor::appType, series.size());
                    my_dataHandler	-> add_mscDatagroup (series); 
                    series. resize (0);
                    break;
            }
        }
        boi+=crc_chk_len+2; 
        packet_number++;
    }   
    return;
}


void dataProcessor::processPacketStream() {
        /*  Loop through the Application Data Table (stored in the outVector[]) for further processing
        As a result of the 2.448 Bytes FEC Frame (2.256 Application Data Table + 192 RS Data Field)
        There should be 2.256 bytes in the outVector to be parsed, before passing to the dataHandler */
    fprintf(stderr, "FEC::applyFEC::DG=%s::MSCDG[%ld]\n", dataProcessor::DGflag?"Y":"N", outVector.size());
    // Feierabend für heute, Parser-Funktion morgen! //
    // my_dataHandler	-> add_mscDatagroup (series)
    
    uint8_t dfoff; // Data field offset
    uint8_t MSCIdx; // Continuity index
    for (size_t j = 0; j < 94; j++) { // 2.256 / 24 = maximum 94 data group packets to process
        if (j * 24 + 4 >= outVector.size()) {
            fprintf(stderr, "!!! DBG::AddressSanitizer1: Index out of bounds: j=%ld\n", j);
            break;
        } else {
            uint8_t cntidx = (outVector[j * 24] & 0x30) >> 4;        				 // Continuity index
            uint8_t flflg = (outVector[j * 24] & 0x0c) >> 2;         				 // First/Last flag
            uint16_t padr = (outVector[j * 24] & 0x03) << 8 | outVector[j * 24 + 1]; // Packet address
            uint8_t udlen = outVector[j * 24 + 2] & 0x7f;            				 // Useful data length
            uint16_t pcrc = (outVector[j * 24 + 22]) << 8 | outVector[j * 24 + 23];  // Packet CRC

            if (udlen > 0 && pcrc != 0 && check_crc_bytes(&(outVector.data())[j * 24], 22)) {
                switch (flflg) {
                    case 2: { // First data group packet
                        //fprintf(stderr, "DBG::FDG: First data group packet\n");
                        uint8_t extfld = (outVector[j * 24 + 3] & 0x80) >> 7; // Extension Field Present Flag (see ETSI EN 300 401 - MSC data group header)
                        uint8_t segfld = (outVector[j * 24 + 3] & 0x20) >> 5; // Segment Flag
                        uint8_t uacfld = (outVector[j * 24 + 3] & 0x10) >> 4; // User Access Field
                        MSCIdx = (outVector[j * 24 + 4] & 0xf0) >> 4; // Continuity index
                        dfoff = 2 * (extfld + segfld);                        // Data field offset
                        dfoff += uacfld * ((outVector[j * 24 + 4 + dfoff] & 0x0f) + 1); // inlcude User Access Field, if present

                        //uint8_t crcflg = (outVector[j * 24 + 3] & 0x40) >> 6; // CRC Flag
                        //uint8_t dgtype = (outVector[j * 24 + 3] & 0x0f);      // Data group type
                        //uint8_t cnt =    (outVector[j * 24 + 4] & 0xf0) >> 4;      // Data group type
                        //uint8_t rep =    (outVector[j * 24 + 4] & 0x0f);      // Repetition index
                        //fprintf(stderr, "DBG::FDG: ext=%d crc=%d seg=%d uac=%d dgt=%d cnt=%d rep=%d dfoff=%d\n", extfld, crcflg, segfld, uacfld, dgtype, cnt, rep, dfoff);
                        // Common SN=15 offset=480 FIC=100
                        // AdV  DBG::FDG: ext=0 crc=1 seg=0 uac=0 dgt=0 cnt=5 rep=0 dfoff=0
                        // TPEG DBG::FDG: ext=0 crc=1 seg=0 uac=0 dgt=0 cnt=0 rep=0 dfoff=0
                        /* ETSI TS 101 756 table 16 -> reference 6.3.6 of ETSI EN 300 401
                            User App Type = 4 -> ETSI TS 103 551 [8] -> TPEG
                            https://www.etsi.org/deliver/etsi_ts/103500_103599/103551/01.01.01_60/ts_103551v010101p.pdf
                            The MSC data groups shall be transported using the TDC in a packet mode service component with
                            data groups, as specified in ETSI TS 101 759 [3], clause 4.1.2.

                        */
                        // Check data group sequence
                        if (curMSC != 16 && (MSCIdx != ((curMSC + 1) & 0x0f))) {
                            fprintf(stderr, "*** MSC DG SEQUENCE CORRUPTED cur=%d last=%d !\n", MSCIdx, curMSC);
                        }
                        curMSC = MSCIdx; // Start a new sequence
                        curPI = cntidx;
                        ByteBuf.resize(0);
                        break;
                    }
                    case 0:   // Intermediate data group packet
                    case 1: { // Last data group packet
                        //bool seqchk =(flflg!=2)?(cntidx != ((curPI + 1) & 0x03)):(MSCIdx != ((curMSC + 1) & 0x0f));
                        if (curMSC != 16 && (cntidx != ((curPI + 1) & 0x03))) {
                            fprintf(stderr, "*** MSC DG packet sequence error cur=%d last=%d, flfg=%d !\n", cntidx, curPI,flflg);
                        }
                        curPI = cntidx;
                        break;
                    }
                    case 3: { // Single packet, mostly padding
                        if (padr == 2) {
                            curPI = (curPI + 1) & 3;
                            curMSC = (curMSC + 1) & 0xf;
                        }
                        break;
                    }
                }

                //if (dfoff != udlen && udlen > 0) {
                if (j * 24 + 3 + udlen <= outVector.size()) {
                    // Copy useful data into ByteBuf
                    auto startIt = outVector.begin() + j * 24 + 3 + dfoff;
                    auto endIt   = outVector.begin() + j * 24 + 3 + udlen;
                    //size_t numBytesCopied = std::distance(startIt, endIt);
                    //fprintf(stderr,"DBG::Copy useful data %d bytes into ByteBuf, skipping %d bytes\n",numBytesCopied,dfoff);
                    std::copy(startIt, endIt, std::back_inserter(ByteBuf));

                    if (flflg == 1 || flflg == 3) {
                        uint16_t mscdglen = ByteBuf.size();
                        if (mscdglen > 0 ) {
                            // Check CRC bytes
                            if (check_crc_bytes(&(ByteBuf.data())[0], mscdglen - 2)) {
                                bytesOut(&(ByteBuf.data())[2], mscdglen - 4, 2, ctx);
                            }
                        } else {
                            fprintf(stderr, "ERR::BYTEBUFFER EMPTY! (j=%ld)\n", j);
                        }
                    }
                } else {
                    fprintf(stderr, "DBG::AddressSanitizer2: Index out of bounds: j=%ld\n", j);
                }
            }
        }
    }

}


// Define the TPEG sync word bytes
//const uint8_t syncWord[4] = { 0x47, 0x65, 0x74, 0x54 };  // TPEG sync word: 0x47657454
//const uint8_t syncWord[3] = { 0xff, 0x0f, 0x04 };  // TPEG sync word: 0x47657454
const uint8_t syncWord[7] =   { 0x01, 0x41, 0x0f, 0xf7, 0xcf, 0x78, 0x9c };  // TPEG sync word:
// 28 01 13 00 00 ff 0f 04 4
/* 38 01 13 00 f0 ff 0f 04 19ac4101410ff7cf789c7d935f6c9658
   28 01 13 00 00 ff 0f 04 46f16b01410ff7cf789c7d926b4ce88a
   18 01 13 00 10 ff 0f 04 25e46c01410ff7cf789c7d946d4cb07a */



// Function to find the sync word in frameBytes[]
uint16_t findSyncWord(const uint8_t* frameBytes, uint16_t currentLength) {
    // Iterate through frameBytes to check for the sync word
    for (uint16_t i = 0; i < currentLength; i++) {  // Ensure room for 4 bytes comparison// 15; i++) { // (
        if (frameBytes[i] == syncWord[0] &&
            frameBytes[i + 1] == syncWord[1] &&
            frameBytes[i + 2] == syncWord[2] &&
            frameBytes[i + 3] == syncWord[3] &&
            frameBytes[i + 4] == syncWord[4] &&
            frameBytes[i + 5] == syncWord[5] &&
            frameBytes[i + 6] == syncWord[6]) {
            return i + 1;  // Return 1-based index (position of sync word)
        }
    }

    return 0;  // Return 0 if sync word not found
}

//
// Copy data buffer to series array and check crc of packet
void dataProcessor::handleAppData(uint8_t *data, int16_t length) {
    uint16_t currentLength = frameBytes.size();
    uint16_t ccrc = calc_crc_bits(data, (length +5 - 2) * 8);
    uint16_t pcrc = getBits(data, (length +5 - 2) * 8, 16);
    crcErrors += (ccrc != pcrc)?1:0;
    uint8_t temp; 
    const bool debug = false;
    if (debug) fprintf(stderr, "DBG::handleAppData: length=%d -> |",length);
    
    frameBytes.resize(currentLength + (length+5)); // dataProcessor::pdlen); // including 3 header + data + 2 CRC bytes
    for (uint8_t i = 0; i < (length+5); i++ ) { // dataProcessor::pdlen; i++) {
        temp = 0; for (uint8_t bit = 0; bit < 8; ++bit) {
            temp |= (data[i * 8 + bit] & 1) << (7 - bit);
        }
        frameBytes[currentLength + i] = temp;
        if (debug) { 
            fprintf(stderr, "%02x", temp);
            if (i == length+2) fprintf(stderr,"|");
        }
    } if (debug) fprintf(stderr, "|%s\n",(ccrc!=pcrc)?"X":"");
    return;
    /* Not necessary to check for sync word in the frameBytes, just keep move example
    if (frameBytes.size() < (188 * 12)) return;
    // Move old parts out if necessary
    std::copy(frameBytes.end() - (188 * 12), frameBytes.end(), frameBytes.begin());
    frameBytes.resize(188 * 12); */
}

//
//
//	Really no idea what to do here
void	dataProcessor::handleTDCAsyncstream (uint8_t *data, int16_t length) {
int16_t	packetLength	= (getBits_2 (data, 0) + 1) * 24;
int16_t	continuityIndex	= getBits_2 (data, 2);
int16_t	firstLast	= getBits_2 (data, 4);
int16_t	address		= getBits   (data, 6, 10);
uint16_t command	= getBits_1 (data, 16);
int16_t	usefulLength	= getBits_7 (data, 17);
    fprintf (stderr, "DBG: dataProcessor::handleTDCAsyncstream\n");
	(void)	length;
	(void)	packetLength;
	(void)	continuityIndex;
	(void)	firstLast;
	(void)	address;
	(void)	command;
	(void)	usefulLength;
	if (!check_CRC_bits (data, packetLength * 8))
	   return;
}
