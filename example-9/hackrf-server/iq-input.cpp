#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the hackrf-server
 *    hackrf server is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    hackrf server is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with hackrf server; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<stdint.h>
#include	<ringbuffer.h>
#include	<arpa/inet.h>
#include	<poll.h>
#include	"iq-input.h"

	iq_Input::iq_Input (int port, RingBuffer<uint8_t> *buffer) {
	   this	-> port		= port;
	   this	-> buffer	= buffer;
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

	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
       
//	Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
	   perror("socket failed"); 
	   exit(EXIT_FAILURE); 
	} 
       
//	Forcefully attaching socket to the port 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                      &opt, sizeof(opt))) { 
	   perror("setsockopt"); 
	   exit (EXIT_FAILURE); 
	} 

	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons (port); 

//	Forcefully attaching socket to the specified port
	if (bind (server_fd, (struct sockaddr *)&address,  
                                            sizeof (address)) < 0) { 
	   perror ("bind failed"); 
	   exit (EXIT_FAILURE); 
	} 

	if (listen (server_fd, 3) < 0) { 
	   fprintf (stderr, "listening on port %d", port); 
	   exit (EXIT_FAILURE); 
	} 
}

	iq_Input::~iq_Input (void) {
	if (running. load ()) {
           running. store (false);
           usleep (1000);
           threadHandle. join ();
        }
	close (server_fd);
}

void	iq_Input::start	(void) {
	threadHandle	= std::thread (&iq_Input::run, this);
}

void	iq_Input::stop	(void) {
	if (running. load ()) {
	   running. store (false);
	   usleep (1000);
	   threadHandle. join ();
	}
}

#define	BUF_SIZE	1024

void	iq_Input::run (void) {
int	new_socket;
struct sockaddr_in address;
int	addrlen = sizeof (address);

	if ((new_socket =
	             accept (server_fd, (struct sockaddr *)&address,  
                                               (socklen_t*)&addrlen)) < 0) { 
	   fprintf (stderr, "error in accepting\n"); 
	   return;
	} 

	running. store (true);
	while (running. load ()) {
	   uint8_t lBuffer [BUF_SIZE];
	   struct pollfd fd;
	   fd. fd	= new_socket;
	   fd. events	= POLLIN;
	   switch (poll (&fd, 1, 1000)) {
	      case -1:
	         running. store (false);
	         close (new_socket);
	         return;

	      case 0:
	         if (running. load ())
	            continue;

	      default:
	         break;
	   }
	   int valRead = read (new_socket, lBuffer, BUF_SIZE); 
	   if (valRead < 0)
	      break;
	   buffer	-> putDataIntoBuffer (lBuffer, valRead);
	}
	connected = false;
	// Close the socket before we finish 
	close (new_socket);	
	running. store (false);
}
