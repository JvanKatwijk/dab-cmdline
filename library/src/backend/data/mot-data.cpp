#
/*
 *    Copyright (C) 2015
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
#include	"mot-data.h"
//
//	First attempt to do "something" with the MOT data
//
//	Two cases
//	The "single item" case, where an item is made up of an
//	header together with a body
//	The "directory" case, where a directory of files is maintained
//	to form together a slideshow or a website
//
		motHandler::motHandler (motdata_t motdata_Handler) {
int16_t	i, j;

	this	-> motdata_Handler	= motdata_Handler;
	for (i = 0; i < 16; i ++) {
	   table [i]. ordernumber = -1;
	   for (j = 0; j < 100; j ++)
	      table [i]. marked [j] = false;
	}
	ordernumber	= 1;
	theDirectory	= NULL;
	old_slide	= NULL;
	pictureCount	= 0;
}

	 	motHandler::~motHandler (void) {
}

void	motHandler::process_mscGroup (uint8_t	*data,
	                              uint8_t	groupType,
	                              bool	lastSegment,
	                              int32_t	segmentNumber,
	                              uint16_t	transportId) {
uint32_t segmentSize	= ((data [0] & 0x1F) << 8) | data [1];

	if ((segmentNumber == 0) && (groupType == 3))  // header
	   processHeader (transportId,
	                  &data [2],
	                  segmentSize,
	                  lastSegment);
	else
	if ((segmentNumber == 0) && (groupType == 6)) 	// MOT directory
	    processDirectory (transportId,
	                      &data [2],
	                      segmentSize,
	                      lastSegment);
	else
	if (groupType == 6) 	// fields for MOT directory
	   directorySegment (transportId,
	                     &data [2],
	                     segmentNumber,
	                     segmentSize,
	                     lastSegment);
	else
	if (groupType == 4) {
//	   fprintf (stderr, "grouptype = %d, Ti = %d, sn = %d, ss = %d\n",
//	                     groupType, transportId, segmentNumber, segmentSize);

	   processSegment  (transportId,
	                    &data [2],
	                    segmentNumber,
	                    segmentSize,
	                    lastSegment);
	}
//	else
//	   fprintf (stderr, "grouptype = %d, Ti = %d, sn = %d, ss = %d\n",
//	                     groupType, transportId, segmentNumber, segmentSize);
}
//
//	Process a regular header, i.e. a type 3
//	This strongly resembles the newEntry method that
//	creates a header for an item in a directory
void	motHandler::processHeader (int16_t	transportId,
	                           uint8_t	*segment,
	                           int32_t	segmentSize,
	                           bool		lastFlag) {
uint32_t headerSize     =
             ((segment [3] & 0x0F) << 9) | (segment [4]) | (segment [5] >> 7);
uint32_t bodySize       =
              (segment [0] << 20) | (segment [1] << 12) |
	         (segment [2] << 4 ) | ((segment [3] & 0xF0) >> 4);

uint8_t contentType	= ((segment [5] >> 1) & 0x3F);
uint16_t contentsubType = ((segment [5] & 0x01) << 8) | segment [6];
int32_t	pointer	= 7;
std::string	name 	= std::string ("");

//      If we had a header with that transportId, do not do anything
        if (getHandle (transportId) != NULL) {
           return;
        }

	while (pointer < headerSize) {
	   uint8_t PLI = (segment [pointer] & 0300) >> 6;
	   uint8_t paramId = (segment [pointer] & 077);
	   uint32_t	length;
	   switch (PLI) {
	      case 00:
	         pointer += 1;
	         break;

	      case 01:
	         pointer += 2;
	         break;

	      case 02:
	         pointer += 5;
	         break;

	      case 03:
	         if ((segment [pointer + 1] & 0200) != 0) {
	            length = (segment [pointer + 1] & 0177) << 8 |
	                      segment [pointer + 2];
	            pointer += 3;
	         }
	         else {
	            length = segment [pointer + 1] & 0177;
	            pointer += 2;
	         }
	         if (paramId == 12) {
	            int16_t i;
	            for (i = 0; i < length - 1; i ++) 
	               name. append (1, char (segment [pointer + i + 1]));
	         }
	         pointer += length;
	   } 
	}

        newEntry (transportId, bodySize, contentType, contentsubType, name);
}

void	motHandler::processSegment	(int16_t	transportId,
	                                 uint8_t	*bodySegment,
	                                 int16_t	segmentNumber,
	                                 int32_t	segmentSize,
	                                 bool		lastFlag) {
int16_t	i;

	motElement *handle = getHandle (transportId);
	if (handle == NULL)
	   return;

	if (handle -> marked [segmentNumber])
	   return;

//      Note that the last segment may have a different size
        if (!lastFlag && (handle -> segmentSize == -1))
           handle -> segmentSize = segmentSize;

//	If we only have a "last" segment, we do not need to register
//	the segment size
//	sanity check
	if (segmentNumber * handle -> segmentSize + segmentSize >
	                                                handle -> bodySize)
	   return;

	handle -> segments [segmentNumber]. resize (segmentSize);
	for (i = 0; i < segmentSize; i ++) 
	   handle -> segments [segmentNumber][i] = bodySegment [i];
	
	handle -> marked [segmentNumber] = true;
	if (lastFlag) 
	   handle -> numofSegments = segmentNumber + 1;

	if (isComplete (handle)) {
	   handleComplete (handle);
	}
}

bool	motHandler::isComplete (motElement *p) {
int16_t	i;

	if (p -> numofSegments == -1)
	   return false;

	for (i = 0; i < p ->  numofSegments; i ++)
	   if (!(p -> marked [i]))
	      return false;

	return true;
}

//	we have data for all directory entries
void	motHandler::handleComplete (motElement *p) {
int16_t i;

std::vector<uint8_t> result;
	for (i = 0; i < p -> numofSegments; i ++)
	   result. insert (std::end (result),
	                   std::begin (p -> segments [i]),
	                   std::end (p -> segments [i]));

	if (p -> contentType == 7) {
#ifdef  TRY_EPG
	   epgHandler. decode (result, p -> name);
#endif
	   return;
	}

	if (p -> contentType != 2) {
           if (p -> name != std::string ("")) {
	      checkDir (p -> name);
	      FILE *x = fopen ((p -> name). c_str (), "wb");
	      if (x == NULL)
	         fprintf (stderr, "cannot write file %s\n",
	                            (p -> name). c_str ());
	      else {
	         (void)fwrite (result. data (), 1, p -> bodySize, x);
	         fclose (x);
	      }
	   }
	   return;
	}

	if (motdata_Handler == NULL)
	   return;
	if (old_slide != NULL) {
	   for (i = 0; i < p ->  numofSegments; i ++) {
	      p -> marked [i] = false;
	      p -> segments [i]. clear ();
	   }
	}
	if (p -> name == std::string (""))
	   p -> name = newName ("/tmp/");
	else
	   p -> name = buildName ("/tmp/", p -> name);
	FILE *f	= fopen (p -> name. c_str (), "wb");
	if (f == NULL)
	   return;
	fwrite (result. data (), 1, result. size (), f);
	motdata_Handler (p -> name, p -> contentsubType, NULL);
	fclose (f);
//	the_picture (result, p -> contentsubType, p -> name);
	old_slide	= p;
}

void	motHandler::checkDir (std::string &s) {
}
	
void	motHandler::processDirectory (int16_t	transportId,
                                      uint8_t	*segment,
                                      int32_t	segmentSize,
                                      bool	lastFlag) {
uint32_t directorySize	= ((segment [0] & 0x3F) << 24) |
	                  ((segment [1]       ) << 16) |
	                  ((segment [2]       ) <<  8) | segment [3];
uint16_t numObjects	= (segment [4] << 8) | segment [5];
int32_t	period		= (segment [6] << 16) | 
	                  (segment [7] <<  8) | segment [8];
int16_t segSize		= ((segment [9] & 0x1F) << 8) | segment [10];

	if ((theDirectory != NULL) &&
	                (theDirectory -> transportId == transportId)) 
	   return;		// already in!!

	if (theDirectory != NULL)	// other directory now
	   delete theDirectory;

	theDirectory = new MOT_directory (transportId, segmentSize,
	                                  directorySize, numObjects);
	memcpy (theDirectory -> dir_segments, segment, segmentSize);
	theDirectory -> marked [0] = true;
}

void	motHandler::directorySegment (uint16_t	transportId,
                                      uint8_t	*segment,
                                      int16_t	segmentNumber,
                                      int32_t	segmentSize,
                                      bool	lastSegment) {
int16_t	i;

	if (theDirectory == NULL)
	   return;
	if (theDirectory -> transportId != transportId)
	   return;
	if (theDirectory -> marked [segmentNumber])
	   return;
	if (lastSegment)
	   theDirectory -> num_dirSegments = segmentNumber + 1;
	theDirectory	-> marked [segmentNumber] = true;
	uint8_t	*address = &theDirectory -> dir_segments [segmentNumber *
	                                    theDirectory -> dir_segmentSize];
	memcpy (address, segment, segmentSize);
//
//	we are "complete" if we know the number of segments and
//	all segments are "in"
	if (theDirectory -> num_dirSegments != -1) {
	   for (i = 0; i < theDirectory -> num_dirSegments; i ++)
	      if (!theDirectory -> marked [i])
	         return;
	}
//
//	yes we have all data to build up the directory
	analyse_theDirectory ();
}
//
//	The directory
void	motHandler::analyse_theDirectory (void) {
uint16_t	numObjects	= theDirectory -> numObjects;
uint16_t	currentBase	= 11;	// in bytes
uint8_t	*data			= theDirectory -> dir_segments;
uint16_t extensionLength	= (data [currentBase] << 8) |
	                                            data [currentBase + 1];
int16_t	i;

	currentBase += 2 + extensionLength;
	for (i = 0; i < numObjects; i ++)
	   currentBase = get_dirEntry (i, data, currentBase);
}

int16_t	motHandler::get_dirEntry	(int16_t	index,
	                                 uint8_t	*data,
	                                 uint16_t	currentBase) {
std::string name ("");

uint16_t transportId	=  (data [currentBase] << 8) | data [currentBase + 1];
uint32_t bodySize	=  (data [currentBase + 2] << 20) |
	                   (data [currentBase + 3] << 12) |
	                   (data [currentBase + 4] <<  4) |
	                  ((data [currentBase + 5] & 0xF0) >> 4);
uint16_t headerSize	= ((data [currentBase + 5] & 0x0F) << 9) |
	                   (data [currentBase + 6] << 1) |
	                  ((data [currentBase + 7] >> 7) & 0x01);
uint8_t  contentType	=  (data [currentBase + 7] >> 1) & 0x3F;
uint16_t subType	= ((data [currentBase + 7] & 0x1) << 8) |
	                    data [currentBase + 8];
uint16_t theEnd		= currentBase + 2 + headerSize;

	currentBase	+= 7 + 2;
	while (currentBase < theEnd) {
	   uint8_t PLI = (data [currentBase] & 0300) >> 6;
	   uint8_t paramId = (data [currentBase] & 077);
	   uint16_t	length;
//	   fprintf (stderr, "PLI = %d, paramId = %d\n", PLI, paramId);
	   switch (PLI) {
	      case 00:
	         currentBase += 1;
	         break;
	      case 01:
//	         if (paramId == 10)
//	            fprintf (stderr, "priority = %d\n",
//	                              data [pointer + 1]);
	      
	         currentBase += 2;
	         break;

	      case 02:
//	         if (paramId == 5) 
//	            fprintf (stderr, "triggertime = %d\n",
//	                             data [currentBase + 1] << 24 |
//	                             data [currentBase + 2] << 16 |
//	                             data [currentBase + 3] <<  8 |
//	                             data [currentBase + 4]);
	         currentBase += 5;
	         break;

	      case 03:
	         if ((data [currentBase + 1] & 0200) != 0) {
	            length = (data [currentBase + 1] & 0177) << 8 |
	                      data [currentBase + 2];
	            currentBase += 3;
	         }
	         else {
	            length = data [currentBase + 1] & 0177;
	            currentBase += 2;
	         }
	         if (paramId == 12) {
	            int16_t i;
	            for (i = 0; i < length - 1; i ++) 
	               name. append (1, char (data [currentBase + i + 1]));
	         }
	         currentBase += length;
	   } 
	}
//
//	creating an entry for an object mentioned in the directory
//	strongly resembles creating a standalone entry, some differences though
	newEntry (index, transportId, bodySize,
	          contentType, subType, name);
	return currentBase;
}


motElement	*motHandler::getHandle (uint16_t transportId) {
int16_t	i;
//
//	we first look for the "free" MOT slides, then
//	for the carrousel
	for (i = 0; i < 16; i ++)
	   if (table [i]. ordernumber != -1 &&
	                  table [i]. transportId == transportId)
	      return &table [i];
	if (theDirectory == NULL)
	   return NULL;

	for (i = 0; i < theDirectory -> numObjects; i ++) {
	   if (theDirectory -> dir_proper [i]. ordernumber == -1)
	      continue;
	   if (theDirectory -> dir_proper [i]. transportId == transportId)
	      return &(theDirectory -> dir_proper [i]);
	}
	return NULL;
}
//
//	Handling a plain header is by:
void	motHandler::newEntry (uint16_t	transportId,
	                      int32_t	size,
	                      int16_t	contentType,
	                      int16_t	contentsubType,
	                      std::string	name) {
int16_t		i;
uint16_t	lowest;
int16_t		lowIndex;

	for (i = 0; i < 16; i ++) {
	   if (table [i]. ordernumber == -1) {
	      table [i]. ordernumber	= ordernumber ++;
	      table [i]. transportId	= transportId;
	      table [i]. body. resize (size);
	      table [i]. bodySize	= size;
	      table [i]. contentType	= contentType;
	      table [i]. contentsubType	= contentsubType;
	      table [i]. segmentSize	= -1;
	      table [i]. numofSegments	= -1;
	      table [i]. name		= name;
	      return;
	   }
	}
//
//	table full, delete the oldest one
//
	lowest		= 65377;
	lowIndex	= 0;
	for (i = 0; i < 16; i ++) {
	   if (table [i]. ordernumber < lowest) {
	      lowIndex = i;
	      lowest = table [i]. ordernumber;
	   }
	}

	table [lowIndex]. ordernumber	= ordernumber ++;
	table [lowIndex]. transportId	= transportId;
	table [lowIndex]. body. resize (size);
	table [lowIndex]. bodySize	= size;
	table [lowIndex]. contentType	= contentType;
	table [lowIndex]. contentsubType	= contentsubType;
	table [lowIndex]. segmentSize	= -1;
	table [lowIndex]. numofSegments	= -1;
	table [lowIndex]. name		= name;
}
//
//	handling an entry in a directory is
void	motHandler::newEntry (int16_t	index,
	                      uint16_t	transportId,
	                      int32_t	size,
	                      int16_t	contentType,
	                      int16_t	contentsubType,
	                      std::string	name) {
motElement	*currEntry = &(theDirectory -> dir_proper [index]);

	currEntry -> ordernumber	= ordernumber ++;
	currEntry -> transportId	= transportId;
	currEntry -> body. resize (size);
	currEntry -> bodySize		= size;
	currEntry -> contentType	= contentType;
	currEntry -> contentsubType	= contentsubType;
	currEntry -> segmentSize	= -1;
	currEntry -> numofSegments	= -1;
	currEntry -> name		= name;
}

void	motHandler::the_picture     (std::vector<uint8_t> data,
	                                  int subType, std::string name) {
FILE *f	= fopen (name. c_str (), "wb");
	if (f == NULL)
	   return;
	fwrite (data. data (), 1, data. size (), f);
	motdata_Handler (name. c_str (), subType, NULL);
	fclose (f);
}

std::string motHandler::newName	(const char *dir) {
	pictureCount ++;
	std::string s = std::string (dir);
	std::string temp = std::string ("picture ");
	s . append (temp); s. append (std::to_string (pictureCount));
	return s;
}

std::string motHandler::buildName (const char *dir, std::string name) {
	pictureCount ++;
	std::string s = std::string (dir);
	std::string temp = std::string ("picture ");
	s . append (temp); s. append (std::to_string (pictureCount));
	return s;
}


