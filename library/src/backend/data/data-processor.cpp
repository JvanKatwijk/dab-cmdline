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
uint8_t rsf[12][16];	// Data Field
uint8_t pcf[94];		// Packet CRC Flag

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
	
	Reed-Solomon RS(120, 110, t = 5) shortened code (see note 1), derived from the original
	systematic RS(255, 245, t = 5) code, shall be applied to 110 byte portions of each audio
	super frame to generate an error protected packet.
	The Reed-Solomon code has length 120 bytes, dimension 110 bytes and allows the correction
	of up to 5 random erroneous bytes in a received word of 120 bytes. 
	
	The shortened Reed-Solomon code may be implemented by adding 135 bytes, all set to zero,
	before the information bytes at the input of an RS(255, 245, t = 5) encoder. After the RS
	coding procedure these null bytes shall be discarded, leading to a RS code word of N = 120
	bytes. 
		
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
	this	-> bytesOut		 = p  -> bytesOut_Handler;
	this    -> mscQuality    = p  -> program_quality_Handler; // added S. Juhl
	this	-> ctx			 = ctx;
        RSDims               = 12; 	 // mp4: 8 -> 12 rows
        
        frameBytes.resize(0);
        outVector.resize (RSDims * 188); 	// (RSDims * 110); // 2.256 Bytes Application Data Table
        
        blockFillIndex  = 0;
        blocksInBuffer  = 0;
        curMSC 		    = 16; 
        curPI		    = 0;
        
        frameCount      = 0;
        frameErrors     = 0;
        rsErrors        = 0;
        crcErrors	    = 0;

        frame_quality   = 0;
        rs_quality      = 0;

	fprintf (stderr, "** DBG: dataProcessor: appType=%d FEC=%d DSCTy=%d (", pd -> appType, FEC_scheme, pd -> DSCTy);
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

	packetState	= 0;
}

	dataProcessor::~dataProcessor	(void) {
	delete		my_dataHandler;
}


void	dataProcessor::addtoFrame (uint8_t *outV) {
//	There is - obviously - some exception, that is
//	when the DG flag is on and there are no datagroups for DSCTy5
	/* MSC Quality report // added S. Juhl */
	
	if (++frameCount >= 100) {
			frameCount    = 0;
			frame_quality = 1 * (100 - frameErrors);
			rs_quality    = 1 * (100 - rsErrors);
			if (mscQuality != nullptr)
				mscQuality (frame_quality, rs_quality, 1*(100-crcErrors), ctx);
			frameErrors = crcErrors = rsErrors = 0;
	}

	if ((this -> DSCTy == 5) &&
	    (this -> DGflag)) {			// no datagroups
	      handleTDCAsyncstream (outV, 24 * bitRate);
	} else {
	      handlePackets (outV, 24 * bitRate);	// Data group packets
	}
}

//
// While for a full mix data and audio there will be a single packet in a
// data compartment, for an empty mix, there may be many more
//	TPEG		appType=4	FEC=1 344
//	TPEG_MM		appType=4	FEC=0 152
//	PPP-RTK-AdV	appType=1500 	FEC=1 192
// NOTE CG optimized
void dataProcessor::handlePackets(uint8_t *data, int16_t length) {
    int16_t packetLength = (getBits_2(data, 0) + 1) * 24;  // 24-96 bytes
    uint16_t address = getBits(data, 6, 10);               // 0 -> only for padding
    uint8_t usefulLength = getBits_7(data, 17);            // unsigned binary number of bytes

    uint16_t pcrc = getBits(data, (packetLength - 2) * 8, 16);
    uint32_t zeroc = getBits(data, (packetLength - 4) * 8, 16) + 
                     (getBits(data, (packetLength - 6) * 8, 16) << 16);

    // Handle FEC scheme
    if (FEC_scheme == 1) {
        if ((pcrc == 0 && zeroc == 0) || address == 1022) {
            handleRSDF(data);
            return;
        }
    }

    // Calculate CRC and handle the packet
    uint16_t ccrc = calc_crc_bits(data, (packetLength - 2) * 8);
    Packet2Arr(data, usefulLength, pcrc != ccrc);
    return;
}

// Helper function to process RS data
void dataProcessor::processRSData(uint8_t *data, uint16_t i, uint8_t &temp) {
    temp = 0;
    for (uint8_t j = 0; j < 8; j++) {
        temp = (temp << 1) | (data[(i + 2) * 8 + j] & 0x01);
    }
}

// Handle sequence of FEC packets and RS decoding
// NOTE: CG optimized
void dataProcessor::handleRSDF(uint8_t *data) {
    if (data == nullptr) return;

    const uint16_t RS_BLOCK_SIZE = 2448;  // Total RS data size
    const uint16_t RS_PACKET_SIZE = 22;   // Number of bytes in each RS packet
    const uint16_t FRAME_SIZE = 188 * 12; // Total frame size
    const uint8_t COMPLETE_BLOCKS = 8;    // Number of blocks for complete data
    const uint16_t MAX_ROWS = 12;         // Number of rows in RS matrix

    uint8_t temp = 0;
    uint16_t rs_dt_pos = 0;
    uint8_t Counter = getBits_4(data, 2);
    uint16_t i, col, row;

    // Ensure frameBytes has enough capacity
    if (frameBytes.size() < RS_BLOCK_SIZE) {
        frameBytes.resize(RS_BLOCK_SIZE);
    }

    // Process Reed-Solomon data
    for (i = 0; i < RS_PACKET_SIZE; i++) {
        processRSData(data, i, temp);  // Refactored helper function

        // Assign to frameBytes, skipping padding bytes
        rs_dt_pos = FRAME_SIZE + RS_PACKET_SIZE * Counter + i;
        if (rs_dt_pos < RS_BLOCK_SIZE) {
            frameBytes[rs_dt_pos] = temp;
        }

        // Assign to rsf matrix
        rs_dt_pos = RS_PACKET_SIZE * Counter + i;
        row = rs_dt_pos / MAX_ROWS;
        col = rs_dt_pos % MAX_ROWS;
        if (rs_dt_pos < 192) {
            rsf[col][row] = temp;
        }
    }

    // Early exit if not all parts are received
    if (Counter < COMPLETE_BLOCKS) {
        packetState = 2;
        return;
    }

    // Apply FEC when all parts are received
    if (!frameBytes.empty()) { applyFEC(); }
    return;
}

//
//  Apply reed solomon forward error correction
/*  (204, 188) Reed-Solomon code is a truncated version of RS(255, 239) code. It deletes the
	first 51 symbols of RS(255, 239) code since they are all zeros. Therefore, their design
	and realization methods are similar. RS(204, 188) code is defined on Galois field GF(2^8). */

/* NOTE Reed-Solomon Verification -> https://scholarworks.calstate.edu/downloads/vh53wz89h
		(204, 188) REED-SOLOMON CODE ENCODER/DECODER DESIGN, SYNTHESIS, AND SIMULATION ...
		A graduate project submitted in partial fulfillment of the requirements 
		For the degree of Master of Science in Electrical Engineering
		By Haoyi Zhang
		sample data table for dt[0]=1, dt[1]=2 .. dt[187]=188
		k=51 -> 0xc3e75ac28e7055ab3ff2fb9a015221de */
// NOTE CG optimized
void dataProcessor::applyFEC(void) {
    // Use dynamic memory allocation to avoid frequent stack allocation
    uint8_t* rsIn = new uint8_t[204];
    uint8_t* rsOut = new uint8_t[188];
    uint8_t* rseIn = new uint8_t[188];

    uint16_t j, k;
    uint8_t rsek = 0, base = 0; // rse

    // Helper function to fill rsIn and rseIn
    auto fillRsIn = [&](uint16_t j, uint16_t k) {
        uint32_t index = (base + j + k * RSDims) % (RSDims * 204);
        if (index < frameBytes.size()) {
            rsIn[k] = frameBytes[index];
            if (k < 188) rseIn[k] = rsIn[k];
        } else {
            rsIn[k] = 0;
        }
    };

    // Apply forward error correction
    for (j = 0; j < RSDims; j++) {
        for (k = 0; k < 204; k++) {
            fillRsIn(j, k);
            if (k > 187) {
                rsIn[k] = rsf[j][k - 188];  // Copy RS Data Field
            }
        }
        rsek += ((my_rsDecoder.dec(rsIn, rsOut, 51) < 0) ? 1 : 0); // braces or rsek += rse; necessary
        // Copy the corrected data to outVector
        for (k = 0; k < 188; k++) {
            outVector[j + k * RSDims] = rsOut[k];
        }
    }

    // Post-FEC check for efficiency of RS Decoder
    uint16_t lenbuf = frameBytes.size(); // fprintf(stderr,"*** DBG frameBytes[%d]\n",lenbuf);

    // Loop through the frameBytes for further processing
    for (j = 0; j < lenbuf / 24; j++) {
        uint8_t cntidx = (outVector[j * 24] & 0x30) >> 4;        				 // Continuity index
        uint8_t flflg = (outVector[j * 24] & 0x0c) >> 2;         				 // First/Last flag
        uint16_t padr = (outVector[j * 24] & 0x03) << 8 | outVector[j * 24 + 1]; // Packet address
        uint8_t udlen = outVector[j * 24 + 2] & 0x7f;            				 // Useful data length
        uint16_t pcrc = (outVector[j * 24 + 22]) << 8 | outVector[j * 24 + 23];  // Packet CRC

        if (udlen > 0 && pcrc != 0 && check_crc_bytes(&(outVector.data())[j * 24], 22)) {
            uint8_t dfoff = 0;
            switch (flflg) {
                case 2: { // First data group packet
                    uint8_t extfld = (outVector[j * 24 + 3] & 0x80) >> 7;
                    uint8_t segfld = (outVector[j * 24 + 3] & 0x20) >> 5;
                    uint8_t uacfld = (outVector[j * 24 + 3] & 0x10) >> 4;
                    uint8_t MSCIdx = (outVector[j * 24 + 4] & 0xf0) >> 4;
                    dfoff = 2 * (extfld + segfld);
                    if (uacfld == 1) dfoff += 1 + (outVector[j * 24 + 3 + dfoff + 1] & 0x0f);

                    // Check data group sequence
                    if (curMSC != 16 && (MSCIdx != ((curMSC + 1) & 0xf))) {
                        fprintf(stderr, "*** MSC DG SEQUENCE CORRUPTED cur=%d last=%d !\n", MSCIdx, curMSC);
                    }
                    curMSC = MSCIdx; // Start a new sequence
                    curPI = cntidx;
                    ByteBuf.resize(0);
                    break;
                }
				case 0:   // Intermediate data group packet
                case 1: { // Last data group packet
                    if (curMSC != 16 && cntidx != ((curPI + 1) & 3)) {
                        fprintf(stderr, "*** MSC DG packet sequence error cur=%d last=%d !\n", cntidx, curPI);
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

            if (dfoff != udlen && udlen > 0) {
                // Copy useful data into ByteBuf
                std::copy(outVector.begin() + j * 24 + 3 + dfoff, outVector.begin() + j * 24 + 3 + udlen, std::back_inserter(ByteBuf));

                if (flflg == 1 || flflg == 3) {
                    uint16_t mscdglen = ByteBuf.size();
                    if (check_crc_bytes(&(ByteBuf.data())[0], mscdglen - 2)) {
                        bytesOut(&(ByteBuf.data())[2], mscdglen - 4, 2, ctx);
                    }
                }
            }
        }
    }

    // Clean up and update error count
    frameBytes.resize(0);
    series.resize(0);
    rsErrors += rsek;

    // Free dynamically allocated memory
    delete[] rsIn;
    delete[] rsOut;
    delete[] rseIn;
    return;
}

//
// Copy data buffer to series array 
// NOTE: CG optimized
void dataProcessor::Packet2Arr(uint8_t *data, int16_t length, bool skipseries) {
    int32_t currentLength = series.size();
    uint16_t i;
    uint8_t temp = 0;
    uint16_t packetLength = (getBits_2(data, 0) + 1) * 24; // 24-96 bytes

    // Fill series data buffer
    if (!skipseries) {
        series.resize(currentLength + 8 * length);
        for (i = 0; i < 8 * length; i++) {
            series[currentLength + i] = data[24 + i];
        }
    }

    // Fill FEC frameBytes buffer
    currentLength = frameBytes.size();
    length = packetLength - 5;
    frameBytes.resize(currentLength + (3 + length + 2)); // 3 header + data + 2 CRC
    pcf[(currentLength + length + 5) / 24] = skipseries;

    for (i = 0; i < (3 + length + 2); i++) {
        temp = ((data[i * 8 + 0] & 1) << 7) |
               ((data[i * 8 + 1] & 1) << 6) |
               ((data[i * 8 + 2] & 1) << 5) |
               ((data[i * 8 + 3] & 1) << 4) |
               ((data[i * 8 + 4] & 1) << 3) |
               ((data[i * 8 + 5] & 1) << 2) |
               ((data[i * 8 + 6] & 1) << 1) |
               ((data[i * 8 + 7] & 1) << 0);
        frameBytes[currentLength + i] = temp;
    }

    if (frameBytes.size() < (188 * 12)) return;

    // Move old parts out if necessary
    std::copy(frameBytes.end() - (188 * 12), frameBytes.end(), frameBytes.begin());
    frameBytes.resize(188 * 12);

    return;
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
