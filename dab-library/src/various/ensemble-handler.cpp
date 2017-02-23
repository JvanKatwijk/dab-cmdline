#
/*
 *    Copyright (C) 2015, 2015, 2016
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"ensemble-handler.h"

	ensembleHandler::ensembleHandler (void) {
	ensembleFound	= false;
}

	ensembleHandler::~ensembleHandler	(void) {
}

std::list<std::string> ensembleHandler::data (void) {
	return stationList;
}

int	ensembleHandler::size (void) {
	return stationList. size ();
}

void	ensembleHandler::addtoEnsemble (const std::string &s) {
uint16_t i;

	for (i = 0; i <  s. size () - 4; i ++)
	   if (s. find ("data", i) != std::string::npos)
	      return;
	locker. lock ();
	for (std::list<std::string>::iterator list_iter = stationList. begin ();
	     list_iter != stationList. end (); list_iter ++) {
	   if ((*list_iter).find (s, 0) != std::string::npos) {
	      locker. unlock ();
	      return;
	   }
	}
	stationList.insert (stationList. cend (), s);
	locker. unlock ();
	fprintf (stderr, "program (%d): %X %s in the list\n",
	                                     stationList. size (),
	                                     SId,
	                                     s. c_str ());
}

///	a slot, called by the fib processor
void	ensembleHandler::nameforEnsemble (int id, const std::string &s) {
	(void)id;
	if (ensembleName != s) {
	   ensembleName = s;
	}
	ensembleFound	= true;
}

std::string	ensembleHandler::nameofEnsemble (void) {
	return ensembleName;
}

bool	ensembleHandler::ensembleExists (void) {
	return ensembleFound;
}
	
std::string	ensembleHandler::findService (const std::string &s) {
std::string name;

	locker. lock ();
	for (std::list<std::string>::iterator list_iter = stationList. begin ();
	     list_iter != stationList. end (); list_iter ++) {
	   if ((*list_iter). find (s, 0) != std::string::npos) {
	      name = *list_iter;
	      break;
	   }
	}
	locker. unlock ();
	return name;
}

std::string	ensembleHandler::getProgram (int16_t i) {
std::string name = "";

	locker. lock ();
	for (std::list<std::string>::iterator list_iter = stationList. begin ();
	     list_iter != stationList. end (); list_iter ++) {
	   if (i == 0) {
	      name = *list_iter;
	      break;
	   }
	   i --;
	}
	locker. unlock ();
	return name;
}

void	ensembleHandler::clearEnsemble	(void) {
	locker. lock ();
	stationList. clear ();
	locker. unlock ();
}

