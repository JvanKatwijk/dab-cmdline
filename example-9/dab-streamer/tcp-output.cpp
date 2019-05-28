#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the dab streamer
 *    dab streamer is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dab streamer is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab streamer; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<stdint.h>
#include	<ringbuffer.h>
#include	<arpa/inet.h>
#include	"tcp-output.h"


	tcpOutput::tcpOutput (int port, std::string address) {
	this	-> port		= port;
	this	-> address	= address;
	connected. store (false);
// Variables for writing a server. 
/*
 *	1. Getting the address data structure.
 *	2. Opening a new socket.
 *	3. Bind to the socket.
 *	4. Listen to the socket. 
 *	5. Accept Connection.
 *	6. Receive Data.
 *	7. Close Connection. 
 */
	struct sockaddr_in server;

	//Create socket
	socket_desc = socket (AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
	   fprintf (stderr, "Could not create socket");
	   return;
	}

	memset (&server, '0', sizeof (server));
//	Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_port = htons (port);

//	Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton (AF_INET, address. c_str (),
	                                 &server. sin_addr) <= 0) {
	   fprintf (stderr, "\nInvalid address/ Address not supported \n");
	   return;
	}

//	Bind
	if (connect (socket_desc, (struct sockaddr *)&server,
	                                   sizeof (server)) < 0) {
	   fprintf (stderr, "\nConnection Failed \n");
	   return;
	}
	bufP		= 0;
	connected. store (true);
}

	tcpOutput::~tcpOutput (void) {
	if (connected. load ())
	   close (socket_desc);	
}

void	tcpOutput::sendSample (std::complex<float> datum) {

	if (!connected. load ())
	   return;
	
	int16_t re = real (datum) * 16384;
	int16_t im = imag (datum) * 16384;
	dataBuffer [4 * bufP    ] = (re >> 8) & 0xFF;
	dataBuffer [4 * bufP + 1] = (re & 0xFF);
	dataBuffer [4 * bufP + 2] = (im >> 8) & 0xFF;
	dataBuffer [4 * bufP + 3] = (im & 0xFF);
	bufP ++;
	if (bufP < BUF_SIZE / 4)
	   return;

	try {
	   int status = send (socket_desc, dataBuffer, BUF_SIZE ,0);
	   if (status == -1) {
	      throw (22);
	   }
	}
	catch (int e) {
	   fprintf (stderr, "got an exception, stopping\n");
	   connected. store (false);
	   return;
	}
}
