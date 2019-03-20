#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
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
 *	Simple streaming server, for e.g. epg data and tpg data
 */

#ifndef	__TCP_CONTROLLER__
#define	__TCP_CONTROLLER__

#include	<stdint.h>
#include        <arpa/inet.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<string>
#include	<atomic>
#include        <unistd.h>
#include        <condition_variable>
#include        <mutex>
#include        <chrono>
#include        <queue>
#include        "dab-class.h"
#include        "device-handler.h"
#include	"includes/support/band-handler.h"
#include	"radiodata.h"

#define	Q_CHANNEL	0101
#define	Q_SERVICE	0102
#define	Q_GAIN		0103
#define	Q_QUIT		0100

class	tcpController {
public:
		tcpController	(int,
	                         dabClass *,
                                 deviceHandler	*,
                                 radioData	*);

		~tcpController	(void);
	void	run		(void);
	bool	isRunning	(void);
private:
	std::thread	threadHandle;
	int		serverPort;
	RingBuffer<uint8_t> 	*buffer;
	std::atomic<bool>	running;
	std::atomic<bool>	connected;
	int                     socket_desc;
        struct sockaddr_in server;
        bool                    isReady;
        std::mutex              mutex;
        std::condition_variable condvar;
        std::queue<std::string> messageQ;
	dabClass		*theRadio;
	deviceHandler		*theDevice;
	bandHandler		*theBandHandler;
	radioData		*my_radioData;
	void			handleRequest (uint8_t *, int);
};
#endif

