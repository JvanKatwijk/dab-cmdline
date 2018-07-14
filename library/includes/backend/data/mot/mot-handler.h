#
/*
 *    Copyright (C) 2015 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the dab library
 *    dab library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dab-library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab-library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef	__MOT_HANDLER__
#define	__MOT_HANDLER__
#include	"dab-constants.h"
#include	"virtual-datahandler.h"
#include	<vector>
#include	"dab-api.h"

class	motObject;
class	motDirectory;

class	motHandler:public virtual_dataHandler {
public:
		motHandler	(motdata_t motdataHandler,
	                         void	*ctx);
		~motHandler	(void);
	void	add_mscDatagroup	(std::vector<uint8_t>);
private:
	motdata_t	motdataHandler;
	void		*ctx;
	void		setHandle	(motObject *, uint16_t);
	motObject	*getHandle	(uint16_t);
	int		orderNumber;
	motDirectory	*theDirectory;
};
#endif

