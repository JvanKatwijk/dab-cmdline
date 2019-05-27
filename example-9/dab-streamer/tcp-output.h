#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the fm modulator
 *    fm modulator is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    fm modulator is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with fm modulator; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__TCP_OUTPUT__
#define	__TCP_OUTPUT__

#include	<stdint.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<string>
#include	<thread>
#include	<unistd.h>
#include	<atomic>
#include	<complex>
#include	"ringbuffer.h"
#include	"output-driver.h"

class	tcpOutput : public  outputDriver {
public:
		tcpOutput	(int, std::string);
		~tcpOutput	(void);
	void	sendSample	(std::complex<float>);
	void	run		(void);
private:
	int		port;
	std::string	address;
	std::thread	threadHandle;
	RingBuffer<std::complex<float>> 	*buffer;
	std::atomic<bool>	running;
	std::atomic<bool>	connected;
};
#endif

