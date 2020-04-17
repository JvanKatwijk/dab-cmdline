#
/*
 *    Copyright (C) 2010, 2011, 2012
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
 *
 *    A simple client for rtl_tcp
 */

#include	"rtl_tcp-client.h"
//
	rtl_tcp_client::rtl_tcp_client	(std::string	hostname,
	                                 int32_t	port,
	                                 int32_t	frequency,
	                                 int16_t	gain,
	                                 bool		autogain,
	                                 int16_t	ppm) {
	this	-> hostname	= hostname;
	this	-> basePort	= port;
	this	-> vfoFrequency	= frequency;
	this	-> gain		= gain;
	this	-> autogain	= autogain;
	this	-> ppm		= ppm;
	this	-> theRate	= 2048000;

	theSocket = socket (AF_INET, SOCK_STREAM, 0);
	if (theSocket < 0) {
	   std::cerr << "Error: " << strerror (errno) << std::endl;
	   throw (21);
	}

//	Fill in server IP address
	struct sockaddr_in server;
	int serverAddrLen;
	bzero (&server, sizeof (server));

	struct hostent *host = gethostbyname (hostname. c_str ());
	if (host == NULL) {
	   std::cerr << "Error: " << strerror (errno) << std::endl;
	   throw (22);
	}

	server. sin_family	= AF_INET;
	server. sin_port	= htons (basePort);

//	Print a resolved address of server (the first IP of the host)
	std::cerr << "server address = " <<
	          (host->h_addr_list[0][0] & 0xff) << "." <<
	          (host->h_addr_list[0][1] & 0xff) << "." <<
	          (host->h_addr_list[0][2] & 0xff) << "." <<
	          (host->h_addr_list[0][3] & 0xff) << ", port " <<
	               static_cast<int>(basePort) << std::endl;

//	write resolved IP address of a server to the address structure
	memmove (&(server. sin_addr. s_addr), host -> h_addr_list [0], 4);

//	Connect to the remote server
	int res = connect (theSocket,
	                      (struct sockaddr*) &server, sizeof (server));
	if (res < 0) {
	   std::cerr << "Error: " << strerror (errno) << std::endl;
	   throw (23);
	}
//
//	OK, we are connected,
//	first allocate a large buffer
	theBuffer	= new RingBuffer<uint8_t> (1024 * 1024);
//	and set the parameters right
	fprintf (stderr, "setting the rate to %d\n", theRate);
	sendCommand (0x02, theRate);
	fprintf (stderr, "setting the frequency to %d\n", vfoFrequency);
	sendCommand (0x01, vfoFrequency);
	sendCommand (0x03, autogain);
	fprintf (stderr, "setting the gain to %d\n", gain);
	sendCommand (0x04, 10 * gain);
	sendCommand (0x05, ppm);
//
//	and start the listening thread
	running. store (false);
        threadHandle    = std::thread (&rtl_tcp_client::run, this);
}

	rtl_tcp_client::~rtl_tcp_client (void) {
	stopReader ();

	close (theSocket);
}

void	rtl_tcp_client:: run (void) {

	running. store (true);
	while (running. load ()) {
           uint8_t buffer [1024];
           int res = read (theSocket, buffer, 1024);
           if (res < 0) {
              std::cerr << "Error: " << strerror(errno) << std::endl;
	      running. store (false);
              return;
           }

           theBuffer -> putDataIntoBuffer (buffer, res);
        }
}

//
//	The brave old getSamples. For the dab stick, we get
//	size: still in I/Q pairs, but we have to convert the data from
//	uint8_t to std::complex<float>
int32_t	rtl_tcp_client::getSamples (std::complex<float> *V, int32_t size) { 
int32_t	amount, i;
uint8_t	*tempBuffer = (uint8_t *)alloca (2 * size * sizeof (uint8_t));
//
	amount = theBuffer	-> getDataFromBuffer (tempBuffer, 2 * size);
	for (i = 0; i < amount / 2; i ++)
	    V [i] = std::complex<float> (
	                        (float (tempBuffer [2 * i] - 128)) / 128.0,
	                        (float (tempBuffer [2 * i + 1] - 128)) / 128.0);
	return amount / 2;
}

int32_t	rtl_tcp_client::Samples	(void) {
	return  theBuffer	-> GetRingBufferReadAvailable () / 2;
}
//

//	bitDepth is is used to set the scale for the spectrum
int16_t	rtl_tcp_client::bitDepth	(void) {
	return 8;
}

void	rtl_tcp_client::sendCommand (uint8_t cmd, uint32_t param) {
uint8_t theCommand [5];

	theCommand [0]		= cmd;
	theCommand [4]		= param & 0xFF;
	theCommand [3]		= (param >> 8) & 0xFF;
	theCommand [2]		= (param >> 16) & 0xFF;
	theCommand [1]		= (param >> 24) & 0xFF;
	write (theSocket, &theCommand, 5);
}

void	rtl_tcp_client::stopReader (void) {
	if (running) {
	   running. store (false);
	   threadHandle. join ();
	}
}

