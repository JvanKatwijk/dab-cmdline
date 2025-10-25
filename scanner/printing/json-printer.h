#
/*
 *    Copyright (C) 2025
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the 2-nd DAB scannner and borrowed
 *    for the Qt-DAB repository of the same author
 *
 *    DAB scannner is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB scanner is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB scanner; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#pragma once

#include	"dab-constants.h"
#include	"scanner-printer.h"
#include	<vector>
#include	<stdio.h>

class	json_printer: public scannerPrinter {
public:
		json_printer	(const std::string &);
		~json_printer	();
	void	print		(const std::vector<ensembleDescriptor> &theResult);
	void	close		();
private:
	void	print_ensemble	(const ensembleDescriptor &, std::string, bool);
	void	print_header	();
	void	print_footer	();
	void	print_audioService	(const contentType &);
	void	print_packetService	(const contentType &);
};

