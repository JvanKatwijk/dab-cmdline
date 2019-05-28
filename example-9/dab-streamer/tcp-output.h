#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
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

#ifndef	__TCP_OUTPUT__
#define	__TCP_OUTPUT__

#include	<stdint.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<string>
#include	<unistd.h>
#include	<atomic>
#include	<complex>
#include	"output-driver.h"

#define	BUF_SIZE	1024
class	tcpOutput : public  outputDriver {
public:
		tcpOutput	(int, std::string);
		~tcpOutput	(void);
	void	sendSample	(std::complex<float>);
private:
	int		port;
	std::string	address;
	int socket_desc;

	std::atomic<bool>	connected;
	uint8_t		dataBuffer [BUF_SIZE];
	int		bufP;
};
#endif

