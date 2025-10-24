#
/*
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of dab-cmdline
 *
 *    dab-cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dab-cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab-cmdline; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once

#include	<string>
#include	<vector>
#include	<stdint.h>
#include	<stdio.h>

class Blocks	{
public:
			Blocks		() {}
			~Blocks		() {}
	int		blockNumber;
	int		nrElements;
	std::string	typeofUnit;
	int		frequency;
	std::string	modType;
};

class xmlDescriptor {
public:
	std::string		deviceName;
	std::string		deviceModel;
	std::string		recorderName;
	std::string		recorderVersion;
	std::string		recordingTime;
	int			sampleRate;
	int			nrChannels;
	int			bitsperChannel;
	std::string		container;
	std::string		byteOrder;
	std::string		iqOrder;
	int			nrBlocks;
	std::vector<Blocks> blockList;
			xmlDescriptor	(FILE *, bool *);
			~xmlDescriptor	();
	void		printDescriptor	();
	void		setSamplerate	(int sr);
	void		setChannels 	(int	nrChannels,
	             	                 int	bitsperChannel,
	                                 std::string	ct,
	             	                 std::string	byteOrder);
	void		addChannelOrder (int channelOrder,
	                                 std::string Value);
	void		add_dataBlock (int currBlock,  uint64_t Count,
                                       int  blockNumber, std::string Unit);
	void		add_freqtoBlock	(int blockno, int freq);
	void		add_modtoBlock (int blockno, std::string modType);
};

