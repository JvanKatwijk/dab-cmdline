#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the fm streamer
 *    fm streamer is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    fm streamer is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with fm streamer; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<stdint.h>
#include	<ringbuffer.h>
#include	<arpa/inet.h>
#include	"tcp-output.h"

	tcpOutput::tcpOutput (int port, std::string address) {
	   this	-> port		= port;
	   this	-> address	= address;
	   buffer	= new RingBuffer<std::complex<float>> (4 * 32768);
	   connected. store (false);
	   threadHandle	= std::thread (&tcpOutput::run, this);
	   running. store (true);
}

	tcpOutput::~tcpOutput (void) {
	fprintf (stderr, "tcp taak stopt\n");
	if (running. load ()) {
           running. store (false);
           usleep (1000);
           threadHandle. join ();
        }
	delete buffer;
}

void	tcpOutput::sendSample (std::complex<float> datum) {

	if (!connected. load ())
	   return;
	while (buffer -> GetRingBufferWriteAvailable () < 10)
	   usleep (1000);
	buffer -> putDataIntoBuffer (&datum, 1);
}
#define	BUF_SIZE	1024

void	tcpOutput::run (void) {
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
	int socket_desc;
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

	connected. store (true);
	running. store   (true);
	fprintf (stderr, "connected\n");
	try {
	   uint8_t	localBuffer [BUF_SIZE];
	   std::complex<float> cBuffer [BUF_SIZE / 4];
	   while (running. load ()) {
	      while (running. load () &&
	              (buffer -> GetRingBufferReadAvailable () < BUF_SIZE / 4)) 
	          usleep (1000);
	      int amount = buffer -> getDataFromBuffer (cBuffer, BUF_SIZE / 4);

	      for (int i = 0; i < amount; i ++) {
	         int16_t re = real (cBuffer [i]) * 16384;
	         int16_t im = imag (cBuffer [i]) * 16384;
	         localBuffer [4 * i    ] = (re >> 8) & 0xFF;
	         localBuffer [4 * i + 1] = (re & 0xFF);
	         localBuffer [4 * i + 2] = (im >> 8) & 0xFF;
	         localBuffer [4 * i + 3] = (im & 0xFF);
	      }

	      int status = send (socket_desc, localBuffer, amount * 4 ,0);
	      if (status == -1) {
	         fprintf (stderr, "y");
	         throw (22);
	      }
	   }
	}
	catch (int e) {
	   fprintf (stderr, "got an exception, stopping\n");
	}
	connected = false;
	// Close the socket before we finish 
	close (socket_desc);	
	running. store (false);
}
