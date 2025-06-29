#
/*
 *    Copyright (C) 2015 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library
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
#include	"pad-handler.h"
#include	<cstring>
#include	"charsets.h"
#include	"mot-object.h"
/**
  *	\class padHandler
  *	Handles the pad segments passed on from mp2- and mp4Processor
  */
	padHandler::padHandler	(API_struct *p, void *ctx) {
	this	-> dataOut		= p -> dataOut_Handler;
	this	-> motdata_Handler	= p -> motdata_Handler;
	this	-> ctx			= ctx;
//
//	mscGroupElement indicates whether we are handling an
//	msc datagroup or not.
	mscGroupElement	= false;
	dataGroupLength	= 0;

//	xpadLength tells - if mscGroupElement is "on" - the size of the
//	xpadfields, needed for handling xpads without CI's
	xpadLength	= -1;
	still_to_go	= 0;
	lastSegment	= false;
	firstSegment	= false;
	segmentNumber	= -1;
	currentSlide	= nullptr;
	dynamicLabelText. clear ();
}

	padHandler::~padHandler	(void) {
	if (currentSlide != nullptr)
	   delete currentSlide;
}

//	Data is stored reverse, we pass the vector and the index of the
//	last element of the XPad data.
//	 L0 is the "top" byte of the L field, L1 the next to top one.
void	padHandler::processPAD (uint8_t *buffer,
	                        int16_t last,
	                        uint8_t L1,
	                        uint8_t L0) {
uint8_t	fpadType	= (L1 >> 6) & 03;

	if (fpadType != 00) 
	   return;
//
//	OK, we'll try

	uint8_t x_padInd = (L1 >> 4) & 03;
	uint8_t CI_flag  = L0 & 02;
	switch (x_padInd) {
	   default:
	      break;

	   case  01 :
	      handle_shortPAD (buffer, last, CI_flag);
	      break;

	   case  02:
	      handle_variablePAD	(buffer, last, CI_flag);
	      break;
	}
}
//
//	Since the data is stored in reversed order, we pass
//	on the vector address and the offset of the last element
//	in that vector
void	padHandler::handle_shortPAD (uint8_t *b, int16_t last, uint8_t CIf) {
int16_t	i;

	if (CIf) { // Has CI
	   uint8_t CI = b [last];
	   firstSegment = (b [last - 1] & 0x40) != 0;
	   lastSegment  = (b [last - 1] & 0x20) != 0;
	   uint8_t AcTy = CI & 037; // application type
	   switch (AcTy) {
	      default:
//	         fprintf(stderr, "AcTy: %d\n", AcTy);
	         break;

	      case 0:		// end marker
	         break;

	      case 2:   // start of new fragment, extract the length
	         if (firstSegment && !lastSegment) {
                    segmentNumber   = b [last - 2] >> 4;
                    if ((dataOut != nullptr) && (dynamicLabelText. size () > 0))
                       dataOut (dynamicLabelText. c_str (), ctx);
                    dynamicLabelText. clear ();
                 }
	
	         still_to_go	 = b [last - 1] & 0x0F;
	         shortpadData. resize (0);
                 dynamicLabelText. push_back (b [last - 3]);
	         break;

	      case 3:   // continuation of fragment
                 for (i = 0; (i < 3) && (still_to_go > 0); i ++) {
                    still_to_go --;
                    shortpadData. push_back (b [last - 1 - i]);
                 }

                 if ((still_to_go <= 0) && (shortpadData. size () > 1)) {
                    shortpadData. push_back (0);
	            std::string segmentText =
	                          toStringUsingCharset (
	                                (const char *)(shortpadData. data ()),
	                                (CharacterSet) charSet,
	                                shortpadData. size ());
	            dynamicLabelText. append (segmentText);
                    shortpadData. resize (0);
                 }
                 break;
	   }
	}
	else {	// No CI
           for (i = 0; (i < 4) && (still_to_go > 0); i ++) {
               dynamicLabelText. push_back (b [last - i]);
               still_to_go --;
           }
//	iff we are at the end of the last segment, show the message
//	(but only if there is something to show) and clear the message
	   if ((still_to_go <= 0) && (shortpadData. size () > 0)) {
	      shortpadData. push_back (0);
	      std::string segmentText =
	                          toStringUsingCharset (
	                                (const char *)(shortpadData. data ()),
                                        (CharacterSet) charSet,
                                        shortpadData. size ());
                    dynamicLabelText. append (segmentText);
	      shortpadData. resize (0);
              if ((dataOut != nullptr) && (dynamicLabelText. length () > 0))
                 dataOut (dynamicLabelText. c_str (), ctx);
              dynamicLabelText. clear ();
           }
	}
}
///////////////////////////////////////////////////////////////////////
//
//	Here we end up when F_PAD type = 00 and X-PAD Ind = 02
static
int16_t	lengthTable [] = {4, 6, 8, 12, 16, 24, 32, 48};

//
//	Since the data is reversed, we pass on the vector address
//	and the offset of the last element in the vector,
//	i.e. we start (downwards)  beginning at b [last];
void	padHandler::handle_variablePAD (uint8_t *b,
	                                int16_t last, uint8_t CI_flag) {
int16_t	CI_Index = 0;
uint8_t CI_table [4];
int16_t	i, j;
int16_t	base	= last;	
std::vector<uint8_t> data;

//	If an xpadfield shows with a CI_flag == 0, and if we are
//	dealing with an msc field, the size to be taken is
//	the size of the latest xpadfield that had a CI_flag != 0
	if (CI_flag == 0) {
	   if (mscGroupElement && (xpadLength > 0)) {
	      data. resize (xpadLength);
	      for (j = 0; j < xpadLength; j ++)
	         data [j] = b [last - j];
	      add_MSC_element (data);
	   }
	   return;
	}
//
//	The CI flag in the F_PAD data is set, so we have local CI's
//	7.4.2.2: Contents indicators are one byte long

	while (((b [base] & 037) != 0) && (CI_Index < 4))
	   CI_table [CI_Index ++] = b [base --];

	if (CI_Index < 4) 	// we have a "0" indicator, adjust base
	   base -= 1;

//	The space for the CI's does belong to the Cpadfield, so
//	but do not forget to take into account the '0'field if CI_Index < 4
	if (mscGroupElement) {	
	   xpadLength = 0;
	   for (i = 0; i < CI_Index; i ++)
	      xpadLength += lengthTable [CI_table [i] >> 5];
	   xpadLength += CI_Index == 4 ? 4 : CI_Index + 1;
	}
//
//	Handle the contents
	for (i = 0; i < CI_Index; i ++) {
	   uint8_t appType	= CI_table [i] & 037;
	   int16_t length	= lengthTable [CI_table [i] >> 5];

	   if (appType == 1) {
	      dataGroupLength = ((b [base] & 077) << 8) | b [base - 1];
	      base -= 4;
	      last_appType = 1;
	      continue;
	   }

//	collect data, reverse the reversed bytes
	   data. resize (length);
	   for (j = 0; j < length; j ++)  
	      data [j] = b [base - j];

	   switch (appType) {
	      default:
	         return;	// sorry, we do not handle this

	      case 2:
	      case 3:
	         dynamicLabel ((uint8_t *)(data. data ()),
	                        data. size (), CI_table [i]);
	         break;

	      case 12:
	         new_MSC_element (data);
	         break;

 	      case 13:
	         add_MSC_element (data);
	         break;
	   }

	   last_appType = appType;
	   base -= length;
	   if (base < 0 && i < CI_Index - 1) {
	      fprintf (stderr, "Hier gaat het fout, base = %d\n", base);
	      return;
	   }
	}
}
//
//	A dynamic label is created from a sequence of (dynamic) xpad
//	fields, starting with CI = 2, continuing with CI = 3
void	padHandler::dynamicLabel (uint8_t *data, int16_t length, uint8_t CI) {
static int16_t segmentno           = 0;
static int16_t remainDataLength    = 0;
static bool    isLastSegment       = false;
static bool moreXPad	= false;
int16_t  dataLength	= 0;

	(void)segmentno;
	if ((CI & 037) == 02) {	// start of segment
	   uint16_t prefix = (data [0] << 8) | data [1];
	   uint8_t field_1 = (prefix >> 8) & 017;
	   uint8_t Cflag   = (prefix >> 12) & 01;
	   uint8_t first   = (prefix >> 14) & 01;
	   uint8_t last    = (prefix >> 13) & 01;
	   dataLength	   = length - 2; // The length with header removed

	   if (first) { 
	      segmentno = 1;
	      charSet = (prefix >> 4) & 017;
	      dynamicLabelText. clear ();
	   }
	   else 
	      segmentno = ((prefix >> 4) & 07) + 1;

	   if (Cflag) {		// special dynamic label command
	      // the only specified command is to clear the display
	      dynamicLabelText. clear ();
	   }
	   else {		// Dynamic text length
	      int16_t totalDataLength = field_1 + 1;
	      if (length - 2 < totalDataLength) {
	         dataLength = length - 2; // the length is shortened by header
	         moreXPad   = true;
	      }
	      else {
	         dataLength = totalDataLength;  // no more xpad app's 3
	         moreXPad   = false;
	      }

//	convert dynamic label
	      std::string segmentText = toStringUsingCharset (
	                                 (const char *)&data [2],
	                                 (CharacterSet) charSet,
	                                 dataLength);

	      dynamicLabelText. append (segmentText);

//	if at the end, show the label
	      if (last) {
	         if ((dataOut != nullptr) && !moreXPad) {
	            dataOut (dynamicLabelText. c_str (), ctx);
	                              
	         }
	         else
	            isLastSegment = true;
	      }
	      else 
	         isLastSegment = false;
//	calculate remaining data length
	      remainDataLength = totalDataLength - dataLength;
	   }
	}
	else 
	if (((CI & 037) == 03) && moreXPad) {
	   if (remainDataLength > length) {
	      dataLength = length;
	      remainDataLength -= length;
	   }
	   else {
	      dataLength = remainDataLength;
	      moreXPad   = false;
	   }
	   
	   std::string segmentText = toStringUsingCharset (
	                                     (const char *) data,
	                                     (CharacterSet) charSet,
	                                     dataLength);
	   dynamicLabelText. append(segmentText);
	   if ((dataOut != nullptr) && !moreXPad && isLastSegment) {
	      dataOut (dynamicLabelText. c_str (), ctx);
	   }
	}
}

//
//	Called at the start of the msc datagroupfield,
//	the msc_length was given by the preceding appType "1"
void	padHandler::new_MSC_element (std::vector<uint8_t> data) {

	if ((int)data. size () >= dataGroupLength) {
//	   msc element is single item
	   build_MSC_segment (data);
	   mscGroupElement = false;
//	   show_motHandling (true);
//         fprintf (stderr, "msc element is single\n");
	   return;
	}

	mscGroupElement		= true;
	msc_dataGroupBuffer. clear ();
	msc_dataGroupBuffer     = data;
//	show_motHandling (true);
}

//
void	padHandler::add_MSC_element	(std::vector<uint8_t> data) {
int32_t currentLength = msc_dataGroupBuffer. size ();
//
//      just to ensure that, when a "12" appType is missing, the
//      data of "13" appType elements is not endlessly collected.
	if (currentLength == 0) {
	   return;
	}

	msc_dataGroupBuffer. insert (std::end (msc_dataGroupBuffer),
	                             std::begin (data), std::end (data));
	if ((int)(msc_dataGroupBuffer. size ()) >= dataGroupLength) {
	   build_MSC_segment (msc_dataGroupBuffer);
	   msc_dataGroupBuffer. clear ();
	   mscGroupElement      = false;
//	   show_motHandling (false);
	}
}

void	padHandler::build_MSC_segment (std::vector<uint8_t> data) {
//	we have a MOT segment, let us look what is in it
//	according to DAB 300 401 (page 37) the header (MSC data group)
//	is
int32_t size    = (int)(data. size ()) <
	                 dataGroupLength ? data. size () :
                                                    dataGroupLength;
	if (size <= 2)
	   return;
	uint8_t		groupType	=  data [0] & 0xF;
	uint8_t		continuityIndex = (data [1] & 0xF) >> 4;
	uint8_t		repetitionIndex =  data [1] & 0xF;
	int16_t		segmentNumber	= -1;		// default
	uint16_t	transportId	= 0;		// default
	bool		lastFlag	= false;	// default
	uint16_t	index;

	(void)continuityIndex; (void)repetitionIndex;
	if ((data [0] & 0x40) != 0) {
	   bool res	= check_crc_bytes (data. data (), size - 2);
	   if (!res) {
//	      fprintf (stderr, "crc failed ");
	      return;
	   }
//	   else
//	      fprintf (stderr, "crc success ");
	}

	if ((groupType != 3) && (groupType != 4))
	   return;		// do not know yet

//	extensionflag
	bool	extensionFlag	= (data [0] & 0x80) != 0;
//	if the segmentflag is on, then a lastflag and segmentnumber are
//	available, i.e. 2 bytes more
	index			= extensionFlag ? 4 : 2;
	bool	segmentFlag	=  (data [0] & 0x20) != 0;
	if ((segmentFlag) != 0) {
	   lastFlag		= data [index] & 0x80;
	   segmentNumber	= ((data [index] & 0x7F) << 8) | data [index + 1];
	   index += 2;
	}

//	if the user access flag is on there is a user accessfield
	if ((data [0] & 0x10) != 0) {
	   int16_t lengthIndicator = data [index] & 0x0F;
	   if ((data [index] & 0x10) != 0) { //transportid flag
	      transportId = data [index + 1] << 8 |
	                    data [index + 2];
	      index += 3;
	   }
	   else {
//	      fprintf (stderr, "sorry no transportId\n");
	      return;
	   }
	   index += (lengthIndicator - 2);
	}

	uint32_t segmentSize    = ((data [index + 0] & 0x1F) << 8) |
	                            data [index + 1];
//
//      handling MOT in the PAD, we only deal here with type 3/4
	switch (groupType) {
	   case 3:
	      if (currentSlide == nullptr) {
//	         fprintf (stderr, "creating %d\n", (uint32_t)transportId);
	         currentSlide   = new motObject (motdata_Handler,
	                                         false,
	                                         transportId,
	                                         &data [index + 2],
	                                         segmentSize,
	                                         lastFlag,
	                                         ctx);
	      }
	     else {
	         if (currentSlide -> get_transportId () == transportId)
	            break;
//	         fprintf (stderr, "out goes %u, in comes %u\n",
//	                  currentSlide -> get_transportId (),
//	                  (uint32_t)transportId);
	         delete currentSlide;
	         currentSlide   = new motObject (motdata_Handler,
	                                         false,
	                                         transportId,
	                                         &data [index + 2],
	                                         segmentSize,
	                                         lastFlag,
	                                         ctx);
	      }
	      break;
	  case 4:
	      if (currentSlide == nullptr)
	         return;
	      if (currentSlide -> get_transportId () == transportId) {
//               fprintf (stderr, "add segment %d of  %d\n",
//                                 segmentNumber, transportId);
	         currentSlide -> addBodySegment (&data [index + 2],
	                                         segmentNumber,
	                                         segmentSize,
	                                         lastFlag);
	      }
	      break;

	   default:             // cannot happen
	      break;
	}
}


