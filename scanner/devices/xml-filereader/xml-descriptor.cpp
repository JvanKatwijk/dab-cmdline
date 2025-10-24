#
/*
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of eti-cmdline
 *
 *    eti-cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    eti-cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with eti-cmdline; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	"xml-descriptor.h"
#include	"rapidxml.hpp"

using namespace rapidxml;

	xmlDescriptor::~xmlDescriptor	() {
}

void	xmlDescriptor::printDescriptor	() {
	fprintf (stderr, "sampleRate =	%d\n", sampleRate);
	fprintf (stderr, "nrChannels	= %d\n", nrChannels);
	fprintf (stderr, "bitsperChannel = %d\n", bitsperChannel);
	fprintf (stderr, "container	= %s\n", container. c_str ());
	fprintf (stderr, "byteOrder	= %s\n",
	                              byteOrder. c_str ());
	fprintf (stderr, "iqOrder	= %s\n",
	                              iqOrder. c_str ());
	fprintf (stderr, "nrBlocks	= %d (%d)\n",
	                              nrBlocks,  (int)(blockList. size ()));
	for (int i = 0; i < (int)blockList. size (); i ++)
	   fprintf (stderr, ">>>   %d %d %s %d %s\n",
	                   blockList. at (i). blockNumber,
	                   blockList. at (i). nrElements,
	                   blockList. at (i). typeofUnit. c_str (),
	                   blockList. at (i). frequency,
	                   blockList. at (i). modType. c_str ());
}
	
void	xmlDescriptor::setSamplerate	(int sr) {
	   this	-> sampleRate = sr;
}

void	xmlDescriptor::setChannels 	(int	nrChannels,
	                                 int	bitsperChannel,
	                                 std::string	ct,
	                                 std::string	byteOrder) {
	this	-> nrChannels		= nrChannels;
	this	-> bitsperChannel	= bitsperChannel;
	this	-> container		= ct;
	this	-> byteOrder		= byteOrder;
}

void	xmlDescriptor::addChannelOrder (int channelOrder, std::string Value) {
	if (channelOrder > 1) 
	   return;
	if (channelOrder == 0)	// first element
	   this	-> iqOrder	= Value == "I" ? "I_ONLY" : "Q_ONLY";
	else
	if ((this -> iqOrder == "I_ONLY") && (Value == "Q"))
	   this -> iqOrder = "IQ";
	else
	if ((this -> iqOrder == "Q_ONLY") && (Value == "I"))
	   this -> iqOrder = "QI";
}

void	xmlDescriptor::add_dataBlock (int currBlock,  uint64_t Count,
                                      int  blockNumber, std::string Unit) {
Blocks	b;
	b. blockNumber	= blockNumber;
	b. nrElements	= Count;
	b. typeofUnit	= Unit;
	blockList. push_back (b);
}

void	xmlDescriptor::add_freqtoBlock	(int blockno, int freq) {
	blockList. at (blockno). frequency = freq;
}

void	xmlDescriptor::add_modtoBlock (int blockno, std::string modType) {
	blockList. at (blockno). modType	= modType;
}
//
//	precondition: file exists and is readable.
//	Note that after the object is filled, the
//	file pointer points to where the contents starts
	xmlDescriptor::xmlDescriptor (FILE *f, bool *ok) {
xml_document<> doc;
uint8_t	theBuffer [10000];
int	zeroCount = 0;
//
//	set default values
	sampleRate	= 2048000;
	nrChannels	= 2;
	bitsperChannel	= 16;
	container	= "int16_t";
	byteOrder	= std::string ("MSB");
	iqOrder		="IQ";
	uint8_t		theChar;
	int	index	= 0;
	while (zeroCount < 500) {
	   theChar = fgetc (f);
	   if (theChar == 0)
	      zeroCount ++;
	   else 
	      zeroCount = 0;

	   theBuffer [index++] = theChar;
	}

	*ok	= false;
	doc. parse <0>((char *)theBuffer);

	xml_node<> *root_node = doc. first_node ();
	if (root_node == nullptr) {
	   fprintf (stderr, "failing to extract valid xml text\n");
	   return;
	}

	fprintf (stderr, "rootnode %s\n", root_node -> name ());
	xml_node<> *node = root_node -> first_node ();
	while (node != nullptr) {
	   if (std::string (node -> name ()) == "Recorder") {
	      for (xml_attribute<>*attr = node -> first_attribute ();
	           attr;attr = attr -> next_attribute ()) {
	         if (std::string (attr -> name ()) == "Name")
	            this -> recorderName = std::string (attr -> value ());
	         if (std::string (attr -> name ()) == "Version")
	            this -> recorderVersion = std::string (attr -> value ());
	      }
	   }

	   if (std::string (node -> name ()) == "Device") {
	      for (xml_attribute<>*attr = node -> first_attribute ();
	           attr;attr = attr -> next_attribute ()) {
	         if (std::string (attr -> name ()) == "Name")
	            this -> deviceName = std::string (attr -> value ());
	         if (std::string (attr -> name ()) == "Model")
	            this ->  deviceModel = std::string (attr -> value ());
	      }
	   }

	   if (std::string (node -> name ()) == "Time") {
	      for (xml_attribute<>*attr = node -> first_attribute ();
                   attr;attr = attr -> next_attribute ()) {
                 if (std::string (attr -> name ()) == "Value")
                    this -> recordingTime = std::string (attr -> value ());
	      }
	   }

	   if (std::string (node -> name ()) == "Sample") {
	      xml_node<>* subNode = node -> first_node ();
	      while (subNode != nullptr) {
	         if (std::string (subNode -> name ()) == "Samplerate") {
	            std::string SR = "2048000";
	            std::string Hz = "Hz";
	            for (xml_attribute<>* attr = node -> first_attribute ();
	                 attr; attr = attr -> next_attribute ()) {
	               if (std::string (attr -> name ()) == "Value")
	                  SR = std::string (attr -> value ());
	               if (std::string (attr -> name ()) == "Unit") {
	                  Hz = std::string (attr -> value ());
	                  int factor = Hz == "Hz" ? 1 :
	                            (Hz == "KHz") || (Hz == "Khz") ? 1000 :
	                            1000000;
	                  setSamplerate (std::stoi (SR) * factor);
	               }
	            }
	         }

	         if (std::string (subNode -> name ()) == "Channels") {
	            std::string Amount	= "2";
	            std::string Bits	= "16";
	            std::string Container	= "uint8";
	            std::string Ordering	= "N/A";
	            for (xml_attribute<>* attr = subNode -> first_attribute ();
	                 attr; attr = attr -> next_attribute ()) {
	               if (std::string (attr -> name ()) == "Amount")
	                  Amount = std::string (attr -> value ());
	               if (std::string (attr -> name ()) == "Bits")
	                  Bits   = std::string (attr -> value ());
	               if (std::string (attr -> name ()) == "Container")
	                  Container   = std::string (attr -> value ());
	               if (std::string (attr -> name ()) == "Ordering")
	                  Ordering   = std::string (attr -> value ());
	            }
	            setChannels (std::stoi (Amount),
	                         std::stoi (Bits),
	                         Container,
	                         Ordering);

	            xml_node <>* subsubNode = subNode -> first_node ();
	            int channelOrder = 0;
	            while (subsubNode != nullptr) {
	               if (std::string (subsubNode -> name ()) == "Channel") {
	                  std::string Value = "I/Q";
	                  xml_attribute <>* attr =
	                                       subsubNode -> first_attribute ();
	                  if (std::string (attr -> name ()) == "Value")
	                     Value = std::string (attr -> value ());
	                  addChannelOrder (channelOrder, Value);
	                  channelOrder ++;
	               }
	               subsubNode = subsubNode -> next_sibling ();
	            } 
	         }

	         subNode = subNode -> next_sibling ();
	      }
	   }

	   if (std::string (node -> name ()) == "Datablocks") {
	      this ->  nrBlocks = 0;
	      int currBlock	= 0;
	      xml_node<>* subNode = node -> first_node ();
	      while (subNode != nullptr) {
	         if (std::string (subNode -> name ()) == "Datablock") {
	            fprintf (stderr, "Datablock detected\n");
	            std::string Count	= "100";
	            std::string Number	= "10";
	            std::string Unit	= "Channel";
	            for (xml_attribute<>* attr = subNode -> first_attribute ();
	                 attr; attr = attr -> next_attribute ()) {
	               fprintf (stderr, "attribute %s (%s)\n",
	                          attr -> name (), attr -> value ());
	               if (std::string (attr -> name ()) == "Count")
	                  Count = std::string (attr -> value ());
	               if (std::string (attr -> name ()) == "Number")
	                  Number = std::string (attr -> value ());
	               if (std::string (attr -> name ()) == "Channel")
	                  Unit = std::string (attr -> value ());
	            }
	            char *nix;
	            add_dataBlock (currBlock,
	                         std::strtol (Count. c_str (), &nix, 10),
	                                      std::stoi (Number), Unit);

	            xml_node <>* subsubNode = subNode -> first_node ();
	            while (subsubNode != nullptr) {
	               if (std::string (subsubNode -> name ()) == "Frequency") {
	                  std::string Unit = "Hz";
	                  std::string Value = "200";
	                  for (xml_attribute<>* attr =
	                                       subsubNode -> first_attribute ();
	                       attr; attr = attr -> next_attribute ()) {
	                     if (std::string (attr -> name ()) == "Unit")
	                        Unit = std::string (attr -> value ());
                             if (std::string (attr -> name ()) == "Value")
	                        Value = std::string (attr -> value ());
	                  }
	                  int Frequency =
	                       Unit == "Hz" ? std::stoi (Value) :
	                                  ((Unit == "KHz") | (Unit == "Khz")) ?
	                                     std::stoi (Value) * 1000 :
	                                     std::stoi (Value) * 1000000;
	                  add_freqtoBlock (currBlock, Frequency);
	               }

	               if (std::string (subsubNode -> name ()) == "Modulation") {
	                  for (xml_attribute<>* attr =
	                                  subsubNode -> first_attribute ();
	                      attr; attr = attr -> next_attribute ()) {
	                     if (std::string (attr -> name ()) == "Value") {
	                        std::string Value = 
	                                     std::string (attr -> value ());
	                        add_modtoBlock (currBlock, Value);
	                     }
                          }
	               }
	               subsubNode = subsubNode -> next_sibling ();
	            }
	            currBlock ++;
	         }
	         subNode = subNode -> next_sibling ();
	      }
	      nrBlocks = currBlock;
	   }
	   node	= node -> next_sibling ();
	}
	*ok	= nrBlocks > 0;
	printDescriptor ();
}

