
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

#include	"label-handler.h"

	labelHandler::labelHandler (void) {
	threadHandle	= std::thread (&labelHandler::threadFunction, this);
}

	labelHandler::~labelHandler (void) {
	running		= false;
	messageNotifier. notify_one ();
	threadHandle. join ();
}

void	labelHandler::showLabel (const std::string &s) {
	std::unique_lock<std::mutex>lock (qmutex);
	locker. lock ();
	buffer. append (s);
	locker. unlock ();
	messageNotifier. notify_one ();
}

void	labelHandler::threadFunction (void) {
int	actualLength;
	running	= true;

	while (running) {
	   std::unique_lock<std::mutex>lock (qmutex);
	   messageNotifier. wait (lock);
	   locker. lock ();
	   actualLength = buffer. size ();
	   if (actualLength > 20)
	      buffer. erase (0, actualLength - 20);
	   fprintf (stderr, "%s \r", buffer. c_str ());
	   locker. unlock ();
	}
}

	
