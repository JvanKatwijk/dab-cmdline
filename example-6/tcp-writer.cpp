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
 *
 *	This server is used to transfer (some of the) output 
 *	- coming to main through callbacks -- to a client controling
 *	the radio
 */

#include	<stdint.h>
#include	"tcp-writer.h"
#include	<vector>
/*
 *	1. Getting the address data structure.
 *	2. Opening a new socket.
 *	3. Bind to the socket.
 *	4. Listen to the socket. 
 *	5. Accept Connection.
 *	6. Receive Data.
 *	7. Close Connection. 
 */
	tcpWriter::tcpWriter (int port) {
//	Create socket
	   socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	   if (socket_desc == -1) {
	      fprintf (stderr, "Could not create socket");
	      return;
	   }

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

	running. store   (false);
	connected. store (false);
	threadHandle	= std::thread (&tcpWriter::run, this);
}

	tcpWriter::~tcpWriter (void) {
        running. store (false);	
	if (threadHandle. joinable ()) {
           threadHandle. join ();
        }
}
//
//	Note that very long strings are to be broken up in segments
void	tcpWriter::sendData (uint8_t key, std::string message) {
std::string m;
int length	= message. length ();
const char *s	= message. c_str ();
uint8_t	*packet;
int	i;

	if (!connected)
	   return;
	
	packet = new uint8_t [length + 4];
	packet [0] = 0xFF;
	packet [1] = length;
	packet [2] = key;
	for (i = 0; i < length; i ++)
	   packet [3 + i] = s [i];
	packet [length + 3] = 0;
	std::lock_guard<std::mutex> guard(mutex);
	messageQ. push (packet);
	condvar.notify_one ();
}

void	tcpWriter::run (void) {
struct sockaddr_in client;
int	client_sock;
int	c;

	running. store (true);
	listen (socket_desc , 3);
	while (running. load ()) {
	   fprintf (stderr, "I am now accepting connections ...\n");
//	Accept a new connection and return back the socket desciptor 
	   c = sizeof(struct sockaddr_in);
     
//	accept connection from an incoming client
	   client_sock = accept (socket_desc, 
	                         (struct sockaddr *)&client,
	                         (socklen_t*)&c);
	   if (client_sock < 0) {
	      perror("accept failed");
	      return;
	   }
	   fprintf (stderr, "Connection accepted");
	   connected. store (true);
	   try {
	      auto sec = std::chrono::seconds (1);
	      while (running. load ()) {
	         std::unique_lock<std::mutex> ulock(mutex);
	         while (messageQ. empty () && running. load ())
	            condvar.wait_for (ulock, 2 * sec);
	         if (!running. load ())
	            mutex. unlock ();
	         while (!messageQ. empty () && running. load ()) {
	            int status = send (client_sock,
	                                messageQ. front (), 
	                                messageQ. front () [1] + 4, 0);
	            delete messageQ. front ();
	            messageQ. pop();
	            if (status == -1) {
	               throw (22);
	            }
	         }
	      }
	   }
	   catch (int e) {}
	}
	// Close the socket before we finish 
	close (socket_desc);	
}
