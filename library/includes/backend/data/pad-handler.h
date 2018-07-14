#
/*
 *    Copyright (C) 2013
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

#ifndef	__PAD_HANDLER__
#define	__PAD_HANDLER__

#include	<stdint.h>
#include	<string>
#include	<vector>
#include	"dab-api.h"

class	motObject;

class	padHandler {
public:
		padHandler	(dataOut_t, motdata_t, void *);
		~padHandler	(void);
	void	processPAD	(uint8_t *, int16_t, uint8_t, uint8_t);
private:
	dataOut_t	dataOut;
	motdata_t	motdata_Handler;
	void		*ctx;
	void		handle_variablePAD	(uint8_t *, int16_t, uint8_t);
	void		handle_shortPAD		(uint8_t *, int16_t, uint8_t);
	void		dynamicLabel		(uint8_t *, int16_t, uint8_t);
	void		new_MSC_element 	(std::vector<uint8_t>);
	void		add_MSC_element		(std::vector<uint8_t>);
	void		build_MSC_segment	(std::vector<uint8_t>);
	bool		pad_crc			(uint8_t *, int16_t);

	std::string	dynamicLabelText;
	std::vector<uint8_t> shortpadData;
	int16_t		charSet;
	uint8_t		last_appType;
	bool		mscGroupElement;
	int16_t		xpadLength;
	int16_t		still_to_go;		// for short pad fragments
	motObject	*currentSlide;
	bool		firstSegment;
	bool		lastSegment;
	int16_t		segmentNumber;
//      dataGroupLength is set when having processed an appType 1
        int dataGroupLength;
//
//      The msc_dataGroupBuffer is - as the name suggests - used for
//      assembling the msc_data group.
        std::vector<uint8_t> msc_dataGroupBuffer;
};

#endif
