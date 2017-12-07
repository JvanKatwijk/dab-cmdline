#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the  DAB-library
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

#include	<stdint.h>
#include	"tcp-controller.h"

// Variables for writing a server. 
/*
 *	1. Getting the address data structure.
 *	2. Opening a new socket.
 *	3. Bind to the socket.
 *	4. Listen to the socket. 
 *	5. Accept Connection.
 *	6. Receive Data.
 *	7. Close Connection. 
 *
 *	The controller "listens" to the port as given and
 *	eecutes the commands for setting the radio
 */

	tcpController::tcpController (int		port,
	                              dabClass		*theRadio,
	                              deviceHandler	*theDevice,
	                              radioData		*my_radioData) {
	serverPort		= port;
	this	-> theRadio	= theRadio;
	this	-> theDevice	= theDevice;
	this	-> my_radioData	= my_radioData;

	this	-> theBandHandler = new bandHandler ();

//	Create socket
	socket_desc = socket (AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
	   fprintf (stderr, "Could not create socket");
	   return;
	}
	fprintf (stderr, "Socket created");
//	Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons (port);
	
//	Bind
	if (bind (socket_desc,
	          (struct sockaddr *)&server , sizeof(server)) < 0) {
//	print the error message
	   perror ("bind failed. Error");
	   return;
	}

	running. store (false);
//	Listen
	threadHandle	= std::thread (&tcpController::run, this);
}

	tcpController::~tcpController (void) {
	if (running. load ()) {
           running. store (false);
           usleep (1000);
           threadHandle. join ();
        }
	delete buffer;
}

bool	tcpController::isRunning	(void) {
	return running. load ();
}

#define	BUF_SIZE	1024

void	tcpController::run (void) {
struct sockaddr_in client;
int	client_sock;
int	c;

	fprintf (stderr, "I am now accepting connections ...\n");
	running. store (true);
	listen (socket_desc , 3);
	while (running. load ()) {
//	Accept a new connection and return back the socket desciptor 
	   c = sizeof (struct sockaddr_in);
     
//	accept connection from an incoming client
	   client_sock = accept (socket_desc, 
	                         (struct sockaddr *)&client,
	                         (socklen_t*)&c);
	   if (client_sock < 0) {
	      perror("accept failed");
	      return;
	   }
	   fprintf (stderr, "Connection accepted");
	   try {
	      uint8_t	localBuffer [BUF_SIZE];
	      while (running. load ()) {
	         int amount = recv (client_sock, localBuffer, amount ,0);
	         if (amount == -1) {
	            throw (22);
	         }
	         handleRequest (localBuffer, amount);
	      }
	   }
	   catch (int e) {
	       fprintf (stderr, "disconnected\n");
	       theDevice	-> stopReader ();
	       theRadio		-> stop       ();
	   }
	}
	// Close the socket before we finish 
	close (socket_desc);	
}

void	tcpController::handleRequest (uint8_t *buffer, int amount) {
int	value;

	if (amount < 2)
	   return;

	switch (buffer [0]) {
	   case Q_QUIT:
	      throw (33);
	      break;

	   case Q_GAIN:
	      my_radioData -> theGain = stoi (std::string ((char *)(&buffer [1])));
	      theDevice	-> setGain (my_radioData -> theGain);
	      break;

	   case Q_SERVICE:
	      my_radioData -> serviceName = std::string ((char *)(&buffer [1]));
	      if (theRadio -> is_audioService (my_radioData -> serviceName))
	         theRadio -> dab_service (my_radioData -> serviceName);
	      break;

	   case Q_CHANNEL:
	      theRadio	-> stop ();
	      theDevice	-> stopReader ();
	      my_radioData -> theChannel = std::string ((char *)(&buffer [1]));
	      {  int frequency = theBandHandler -> Frequency (my_radioData -> theBand,
	                                              my_radioData -> theChannel);
	         theDevice	-> setVFOFrequency (frequency);
	      }
	      theRadio	-> startProcessing ();
	      theDevice	-> restartReader ();
	      break;

	   default:
	      break;
	}
}




