#
/*
 *    Copyright (C) 2015
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
 *	Communication via network to a DAB receiver to 
 *	show the tdc data
 */

#include	<QSettings>
#include	<QLabel>
#include	<QMessageBox>
#include	<QtNetwork>
#include	<QTcpSocket>
#include	<QHostAddress>
#include	"client.h"
//

	Client::Client (QWidget *parent):QDialog (parent) {
int16_t	i;
	setupUi (this);
	connected	= false;
	connect (connectButton, SIGNAL (clicked (void)),
	         this, SLOT (wantConnect (void)));
	connect (terminateButton, SIGNAL (clicked (void)),
	         this, SLOT (terminate (void)));
	state	-> setText ("waiting to start");

	connectionTimer	= new QTimer ();
	connectionTimer	-> setInterval (1000);
	connect (connectionTimer, SIGNAL (timeout (void)),
	         this, SLOT (timerTick (void)));
}
//

	Client::~Client	(void) {
	connected	= false;
}
//
void	Client::wantConnect (void) {
QString ipAddress;
int16_t	i;
QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

	if (connected)
	   return;
	// use the first non-localhost IPv4 address
	for (i = 0; i < ipAddressesList.size(); ++i) {
	   if (ipAddressesList.at (i) != QHostAddress::LocalHost &&
	      ipAddressesList. at (i). toIPv4Address ()) {
	      ipAddress = ipAddressesList. at(i). toString();
	      break;
	   }
	}
	// if we did not find one, use IPv4 localhost
	if (ipAddress. isEmpty())
	   ipAddress = QHostAddress (QHostAddress::LocalHost).toString ();
	hostLineEdit -> setText (ipAddress);

	hostLineEdit -> setInputMask ("000.000.000.000");
//	Setting default IP address
	state	-> setText ("Give IP address, return");
	connect (hostLineEdit, SIGNAL (returnPressed (void)),
	         this, SLOT (setConnection (void)));
}

//	if / when a return is pressed in the line edit,
//	a signal appears and we are able to collect the
//	inserted text. The format is the IP-V4 format.
//	Using this text, we try to connect,
void	Client::setConnection (void) {
QString s	= hostLineEdit -> text ();
QHostAddress theAddress	= QHostAddress (s);
int32_t	basePort;
	basePort	= 8888;
	disconnect (hostLineEdit, SIGNAL (returnPressed (void)),
	            this, SLOT (setConnection (void)));
//
//	The streamer will provide us with the raw data
	streamer. connectToHost (theAddress, basePort);
	if (!streamer. waitForConnected (2000)) {
	   QMessageBox::warning (this, tr ("client"),
	                                   tr ("setting up stream failed\n"));
	   return;
	}

	connected	= true;
	state -> setText ("Connected");
	connect (&streamer, SIGNAL (readyRead (void)),
	         this, SLOT (readData (void)));
	connectionTimer	-> start (1000);
}

//	These functions are typical for network use
void	Client::readData	(void) {
QByteArray d;
int16_t	i;

	d. resize (4 * 512);
	while (streamer. bytesAvailable () > 4 * 512) {
	   streamer. read (d. data (), d. size ());
	   for (i = 0; i < d. size (); i ++)
	      handle ((uint8_t)(d [i]));
	}
}



#define	SEARCH_HEADER	0
#define	HEADER_FOUND	1

int	searchState = SEARCH_HEADER;

void	Client::handle (uint8_t d) {
int16_t i;
	if (searchState == SEARCH_HEADER) {
	   headertester. shift (d);
	   if (headertester. hasHeader ()) {
	      toRead = headertester. length ();
	      searchState = HEADER_FOUND;
	      headertester. reset ();
	   }
	   return;
	}
	toRead --;
//	handle the data element
	if (toRead == 0)
	   searchState = SEARCH_HEADER;
}

void	Client::terminate	(void) {
	if (connected) {
	   streamer. close ();
	}
	accept ();
}

void	Client::timerTick (void) {
	if (streamer. state () == QAbstractSocket::UnconnectedState) {
	   state	-> setText ("not connected");
	   connected	= false;
	   connectionTimer	-> stop ();
	}
}
	   
