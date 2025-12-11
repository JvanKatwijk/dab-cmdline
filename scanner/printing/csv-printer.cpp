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

#include	"csv-printer.h"
#include	"dab-tables.h"

	csv_printer::csv_printer	(const std::string &fileName):
	                                            scannerPrinter (fileName) {
}

	csv_printer::~csv_printer	() {
}

void	csv_printer::close		() {
}

void	csv_printer::print (const std::vector<ensembleDescriptor> &theResults) {
typedef struct {
	std::string ensembleName;
	std::string channel;
} printed;
std::vector<printed> done;

	int nrEnsembles	= theResults. size ();
	int counter	= 1;
	print_header ();
	for (auto &ens: theResults) {
	   std::string name = ens. ensemble;
	   bool second	= false;
//	   for (auto &s : done) {
//	      if (s. ensembleName == ens. ensemble) {
//	         second = true;
//	         print_ensemble (ens, s. channel, counter == nrEnsembles);
//	         printed x;
//	         x . ensembleName = ens. ensemble;
//	         x. channel	= ens. channel;
//	         done. push_back (x);
//	      }
//	   }
	   if (!second) {
	      print_ensemble (ens, "", counter == nrEnsembles);
	      printed x;
	      x . ensembleName = ens. ensemble;
	      x. channel	= ens. channel;
	      done. push_back (x);
	   }
	   counter++;
	}
	print_footer ();
}
	
void	csv_printer::print_ensemble (const ensembleDescriptor &ens,
	                                        std::string s, bool last) {
	
	fprintf (theFile, "\n%s; %X;%s; snr %d",
	                   ens. channel. c_str (), ens. ensembleId,
	                   ens. ensemble. c_str (), ens. snr);
	if (s != "")
	   fprintf (theFile, " see %s;n", s. c_str ());
	else	
	   fprintf (theFile, "\n");
	if (ens. transmitterData. size () > 0) {
	   int nrTransmitters = ens. transmitterData. size ();
	   int counter = 1;
	   for (auto &c: ens. transmitterData) {
	      fprintf (theFile,
	           ";;(%d-%d);%s; %f;%f\n",
	                          c. mainId, c. subId,
	                          c. transmitterName. c_str (),
	                          c. latitude, c. longitude);
	   }
	}
	fprintf (theFile, "\nAudio services\ntype;;serviceId;service name;subchannelId;start address;length (CU); bit rate;DAB/DAB+; genre; prot level; code rate; language\n\n");

	if (s == "") {
	   bool first = true;
	   for (auto &as: ens. audioServices) {
	      if (!first)
	         fprintf (theFile, ",\n");
	      first = false;
	      print_audioService (as);
	   }
	   for (auto &ps: ens. packetServices) {
	      fprintf (theFile, ",\n");
	      print_packetService (ps);
	   }
	}
}

void	csv_printer::print_header	() {
}

void	csv_printer::print_footer	() {
}

void	csv_printer::print_audioService (const contentType &as) {
	fprintf (theFile, 
	         ";;%X;%s;%d;%d;%d;%d;%s;%s;%s;%s;%s;\n",
                     as. SId,
                     as. serviceName. c_str (),
                     as. subChId,
                     as. startAddress,
                     as. length,
                     as. bitRate,
                     getASCTy  (as. ASCTy_DSCTy),
                     getProgramType (as.  programType),
                     as. protLevel. c_str (),
                     as. codeRate. c_str (),
                     getLanguage (as. language));
}

void	csv_printer::print_packetService (const contentType &ps) {
	fprintf (theFile, 
	         ";;%X;%s;%d;%d;%d;%d;%s;%s;%s;%s;%s;\n",
	             ps. SId,
	             ps. serviceName. c_str (),
	             ps. subChId,
	             ps. startAddress,
	             ps. length,
	             ps. bitRate,
	             getFECscheme (ps. FEC_scheme),
	             ps. protLevel. c_str (),
	             getUserApplicationType (ps. appType),
	             getDSCTy (ps. ASCTy_DSCTy),
	             ps. primary ? "no": "yes");
}
	
