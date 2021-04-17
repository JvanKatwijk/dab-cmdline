#
/*
 *    Copyright (C) 2014 .. 2017
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
 * 	superframer for the SDR-J DAB+ receiver (and derived programs)
 * 	This processor handles the whole DAB+ specific part
 ************************************************************************
 *	may 15 2015. A real improvement on the code
 *	is the addition from Stefan Poeschel to create a
 *	header for the aac that matches, really a big help!!!!
 *	bitWriter and the text of build_aacFile is from DABlin
 ************************************************************************
    DABlin - capital DAB experience
    Copyright (C) 2015-2019 Stefan Pöschel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include	"mp4processor.h"
#include	<cstring>
//
#include	"charsets.h"
#include	"pad-handler.h"

#include	<stdlib.h>
#include	<stdint.h>
#include	<vector>

class BitWriter {
private:
        std::vector<uint8_t> data;
        size_t byte_bits;
public:
        BitWriter() {Reset();}

void	Reset () {
	data.clear();
	byte_bits = 0;
}

void	AddBits (int data_new, size_t count) {
	while (count > 0) {
//	add new byte, if needed
	   if (byte_bits == 0)
	      data.push_back (0x00);

	   size_t copy_bits = std::min(count, 8 - byte_bits);
	   uint8_t copy_data =
	       (data_new >> (count - copy_bits)) & (0xFF >> (8 - copy_bits));
	   data.back() |= copy_data << (8 - byte_bits - copy_bits);

	   byte_bits = (byte_bits + copy_bits) % 8;
	   count -= copy_bits;
	}
}

void	AddBytes (const uint8_t *data, size_t len) {
	for(size_t i = 0; i < len; i++)
	   AddBits (data[i], 8);
}

const std::vector<uint8_t> GetData() {
	   return data;
	}

void	WriteAudioMuxLengthBytes () {
	size_t len = data.size() - 3;
	data [1] |= (len >> 8) & 0x1F;
	data [2] = len & 0xFF;
}
};

//
//	Now for real
/**
  *	\class mp4Processor is the main handler for the aac frames
  *	the class proper processes input and extracts the aac frames
  *	that are processed by the "faadDecoder" class
  */

	mp4Processor::mp4Processor (int16_t		bitRate,
	                            audioOut_t		soundOut,
	                            dataOut_t		dataOut,
	                            programQuality_t	mscQuality,
	                            motdata_t		motdata_Handler,
	                            void		*ctx):
	                                  my_padHandler (dataOut,
	                                                 motdata_Handler,
	                                                 ctx),
	                                  my_rsDecoder (8, 0435, 0, 1, 10),
	                                  aacDecoder (soundOut, ctx) {

	this	-> bitRate	= bitRate;	// input rate
	this	-> soundOut	= soundOut;
	this	-> mscQuality	= mscQuality;	//
	this	-> ctx		= ctx;
	superFramesize		= 110 * (bitRate / 8);
	RSDims			= bitRate / 8;
	frameBytes. resize (RSDims * 120);	// input
	outVector.  resize (RSDims * 110);
	blockFillIndex	= 0;
	blocksInBuffer	= 0;

	blockFillIndex	= 0;
	blocksInBuffer	= 0;
	frameCount      = 0;
        frameErrors     = 0;
        aacErrors       = 0;
        aacFrames       = 0;
        successFrames   = 0;
        rsErrors        = 0;

	frame_quality	= 0;
	rs_quality	= 0;
	aac_quality	= 0;
}

	mp4Processor::~mp4Processor (void) {
}
//
//	we add vector for vector to the superframe. Once we have
//	5 lengths of "old" frames, we check
void	mp4Processor::addtoFrame (uint8_t *V) {
int16_t	i, j;
uint8_t	temp	= 0;
int16_t	nbits	= 24 * bitRate;
//
//	Note that the packing in the entry vector is still one bit
//	per Byte, nbits is the number of Bits (i.e. containing bytes)
	for (i = 0; i < nbits / 8; i ++) {	// in bytes
	   temp = 0;
	   for (j = 0; j < 8; j ++)
	      temp = (temp << 1) | (V [i * 8 + j] & 01);
	   frameBytes [blockFillIndex * nbits / 8 + i] = temp;
	}
//
	blocksInBuffer ++;
	blockFillIndex = (blockFillIndex + 1) % 5;
//
//	we take the last five blocks to look at
	if (blocksInBuffer >= 5) {
	   if (++frameCount >= 50) {
	      frameCount = 0;
	      frame_quality	= 2 * (50 - frameErrors);
	      if (mscQuality != nullptr)
	         mscQuality (frame_quality, rs_quality, aac_quality, ctx);
	      frameErrors = 0;
	   }

//	OK, we give it a try, check the fire code
	   if (fc. check (&frameBytes [blockFillIndex * nbits / 8]) &&
	       (processSuperframe (frameBytes. data (),
	                           blockFillIndex * nbits / 8))) {
//	since we processed a full cycle of 5 blocks, we just start a
//	new sequence, beginning with block blockFillIndex
	      blocksInBuffer	= 0;
	      if (++successFrames > 25) {
	         rs_quality	= 4 * (25 - rsErrors);
                 successFrames  = 0;
                 rsErrors       = 0;
              }
	   }
	   else {	// virtual shift to left in block sizes
	      blocksInBuffer  = 4;
	      frameErrors ++;
	   }
	}
}
//
bool	mp4Processor::processSuperframe (uint8_t frameBytes [],
	                                 int16_t base) {
uint8_t		num_aus;
int16_t		i, j, k;
uint8_t		rsIn	[120];
uint8_t		rsOut	[110];
stream_parms	streamParameters;

/**	apply reed-solomon error repar
  *	OK, what we now have is a vector with RSDims * 120 uint8_t's
  *	Output is a vector with RSDims * 110 uint8_t's
  */
	for (j = 0; j < RSDims; j ++) {
	   int16_t ler	= 0;
	   for (k = 0; k < 120; k ++) 
	      rsIn [k] = frameBytes [(base + j + k * RSDims) % (RSDims * 120)];
//
	   ler = my_rsDecoder. dec (rsIn, rsOut, 135);
	   if (ler < 0)
	      return false;
	   for (k = 0; k < 110; k ++) 
	      outVector [j + k * RSDims] = rsOut [k];
	}
//
//	OK, the result is N * 110 * 8 bits 
//	bits 0 .. 15 is firecode
//	bit 16 is unused
	streamParameters. dacRate = (outVector [2] >> 6) & 01;	// bit 17
	streamParameters. sbrFlag = (outVector [2] >> 5) & 01;	// bit 18
	streamParameters. aacChannelMode = (outVector [2] >> 4) & 01;	// bit 19
	streamParameters. psFlag   = (outVector [2] >> 3) & 01;	// bit 20
	streamParameters. mpegSurround	= (outVector [2] & 07);	// bits 21 .. 23

//
//      added for the aac file writer
        streamParameters. CoreSrIndex   =
                      streamParameters. dacRate ?
                                    (streamParameters. sbrFlag ? 6 : 3) :
                                    (streamParameters. sbrFlag ? 8 : 5);
        streamParameters. CoreChConfig  =
                      streamParameters. aacChannelMode ? 2 : 1;

        streamParameters. ExtensionSrIndex =
                      streamParameters. dacRate ? 3 : 5;

	switch (2 * streamParameters. dacRate + streamParameters. sbrFlag) {
	   default:		// cannot happen
	   case 0:
	      num_aus = 4;
	      au_start [0] = 8;
	      au_start [1] = outVector [3] * 16 + (outVector [4] >> 4);
	      au_start [2] = (outVector [4] & 0xf) * 256 + outVector [5];
	      au_start [3] = outVector [6] * 16 + (outVector [7] >> 4);
	      au_start [4] = 110 *  (bitRate / 8);
	      break;
//
	   case 1:
	      num_aus = 2;
	      au_start [0] = 5;
	      au_start [1] = outVector [3] * 16 + (outVector [4] >> 4);
	      au_start [2] = 110 * (bitRate / 8);
	      break;
//
	   case 2:
	      num_aus = 6;
	      au_start [0] = 11;
	      au_start [1] = outVector [3] * 16 + (outVector [4] >> 4);
	      au_start [2] = (outVector [4] & 0xf) * 256 + outVector [5];
	      au_start [3] = outVector [6] * 16 + (outVector [7] >> 4);
	      au_start [4] = (outVector [7] & 0xf) * 256 +
	                     outVector [8];
	      au_start [5] = outVector [9] * 16 +
	                     (outVector [10] >> 4);
	      au_start [6] = 110 *  (bitRate / 8);
	      break;
//
	   case 3:
	      num_aus = 3;
	      au_start [0] = 6;
	      au_start [1] = outVector [3] * 16 + (outVector [4] >> 4);
	      au_start [2] = (outVector [4] & 0xf) * 256 +
	                     outVector [5];
	      au_start [3] = 110 * (bitRate / 8);
	      break;
	}
//
//	extract the AU'si and prepare a buffer, sufficiently
//	long for conversion to PCM samples

	for (i = 0; i < num_aus; i ++) {
	   int16_t	aac_frame_length;
//
	   if (au_start [i + 1] < au_start [i]) {
//	      fprintf (stderr, "%d %d\n", au_start [i + 1], au_start [i]);
	      return false;
	   }

	   aac_frame_length = au_start [i + 1] - au_start [i] - 2;
// sanity check
	   if ((aac_frame_length >= 960) || (aac_frame_length < 0)) {
	      return false;
	   }

//	but first the crc check
	   if (check_crc_bytes (&(outVector. data ()) [au_start [i]],
	                        aac_frame_length)) {
	      bool err;
//
//	if there is pad handle it always
	      if (((outVector [au_start [i] + 0] >> 5) & 07) == 4) {
	         int16_t count = outVector [au_start [i] + 1];
#ifdef _MSC_VER
                 uint8_t *buffer = (uint8_t *)_alloca(count);
#else
                 uint8_t buffer [count];
#endif
                 memcpy (buffer, &outVector [au_start [i] + 2], count);
                 uint8_t L0   = buffer [count - 1];
                 uint8_t L1   = buffer [count - 2];
                 my_padHandler. processPAD (buffer, count - 3, L1, L0);
              }
#ifdef	AAC_OUT
	      std::vector<uint8_t> fileBuffer;
	      build_aacFile (aac_frame_length,
	                     &streamParameters,
	                     &(outVector. data () [au_start [i]]),
	                     fileBuffer);
	      if (soundOut != nullptr) 
	         (soundOut)((int16_t *)(fileBuffer. data ()),
	                    fileBuffer. size (), 0, false, nullptr);
#else	
//	we handle the aac -> PMC conversion here
	
	      uint8_t theAudioUnit [2 * 960 + 10];	// sure, large enough
	      memcpy (theAudioUnit,
	                       &outVector [au_start [i]], aac_frame_length);
	      memset (&theAudioUnit [aac_frame_length], 0, 10);

	      int tmp = aacDecoder. MP42PCM (&streamParameters,
	                                     theAudioUnit,
	                                     aac_frame_length);
	      err = tmp == 0;
//	      handle_aacFrame (&outVector [au_start [i]],
//	                       aac_frame_length,
//	                       &streamParameters,
//	                       &err);
	      isStereo (streamParameters. aacChannelMode);
	      if (err) 
	         aacErrors ++;
	      if (++aacFrames > 25) {
	         aac_quality	= 4 * (25 - aacErrors);
	         aacErrors	= 0;
	         aacFrames	= 0;
	      }
#endif
	   }
	   else {
	      fprintf (stderr, "CRC failure with dab+ frame should not happen\n");
	   }
	}
	return true;
}

void	mp4Processor::handle_aacFrame (uint8_t *v,
	                               int16_t frame_length,
	                               stream_parms *sp,
	                               bool	*error) {
uint8_t theAudioUnit [2 * 960 + 10];	// sure, large enough

	memcpy (theAudioUnit, v, frame_length);
	memset (&theAudioUnit [frame_length], 0, 10);

//	if (((theAudioUnit [0] >> 5) & 07) == 4) {
//	   int16_t count = theAudioUnit [1];
//           uint8_t buffer [count];
//           memcpy (buffer, &theAudioUnit [2], count);
//           uint8_t L0   = buffer [count - 1];
//           uint8_t L1   = buffer [count - 2];
//           my_padHandler. processPAD (buffer, count - 3, L1, L0);
//        }

	int tmp = aacDecoder. MP42PCM (sp,
	                               theAudioUnit,
	                               frame_length);
	*error	= tmp == 0;
}

void	mp4Processor::show_frameErrors	(int s) {
	(void)s;
}

void	mp4Processor::show_rsErrors	(int s) {
	(void)s;
}

void	mp4Processor::show_aacErrors	(int s) {
	(void)s;
}

void	mp4Processor::isStereo		(bool b) {
	(void)b;
}

void	mp4Processor::build_aacFile (int16_t	aac_frame_len,
	                             stream_parms *sp,
	                             uint8_t	*data,
	                             std::vector<uint8_t> &fileBuffer) {
BitWriter	au_bw;

	au_bw. AddBits (0x2B7, 11);	// syncword
	au_bw. AddBits (    0, 13);	// audioMuxLengthBytes - written later
//	AudioMuxElement(1)

	au_bw. AddBits (    0, 1);	// useSameStreamMux
//	StreamMuxConfig()

	au_bw. AddBits (    0, 1);	// audioMuxVersion
	au_bw. AddBits (    1, 1);	// allStreamsSameTimeFraming
	au_bw. AddBits (    0, 6);	// numSubFrames
	au_bw. AddBits (    0, 4);	// numProgram
	au_bw. AddBits (    0, 3);	// numLayer

	if (sp  -> sbrFlag) {
	   au_bw. AddBits (0b00101, 5); // SBR
	   au_bw. AddBits (sp -> CoreSrIndex, 4); // samplingFrequencyIndex
	   au_bw. AddBits (sp -> CoreChConfig, 4); // channelConfiguration
	   au_bw. AddBits (sp -> ExtensionSrIndex, 4);	// extensionSamplingFrequencyIndex
	   au_bw. AddBits (0b00010, 5);		// AAC LC
	   au_bw. AddBits (0b100, 3);							// GASpecificConfig() with 960 transform
	} else {
	   au_bw. AddBits (0b00010, 5); // AAC LC
	   au_bw. AddBits (sp -> CoreSrIndex, 4); // samplingFrequencyIndex
	   au_bw. AddBits (sp -> CoreChConfig, 4); // channelConfiguration
	   au_bw. AddBits (0b100, 3);							// GASpecificConfig() with 960 transform
	}

	au_bw. AddBits (0b000, 3);	// frameLengthType
	au_bw. AddBits (0xFF, 8);	// latmBufferFullness
	au_bw. AddBits (   0, 1);	// otherDataPresent
	au_bw. AddBits (   0, 1);	// crcCheckPresent

//	PayloadLengthInfo()
	for (int i = 0; i < aac_frame_len / 255; i++)
	   au_bw. AddBits (0xFF, 8);
	au_bw. AddBits (aac_frame_len % 255, 8);

	au_bw. AddBytes (data, aac_frame_len);
	au_bw. WriteAudioMuxLengthBytes ();
	fileBuffer	= au_bw. GetData ();
}


