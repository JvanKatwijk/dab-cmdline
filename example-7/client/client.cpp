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
#include	"protocol.h"
//

static
const char *channelTable [] =  {"5A",  "5B",  "5C",  "5D",
	                        "7A",  "7B",  "7C",  "7D",
	                        "8A",  "8B",  "8C",  "8D",
	                        "9A",  "9B",  "9C",  "9D",
	                        "10A", "10B", "10C", "10D",
	                        "11A", "11B", "11C", "11D",
	                        "12A", "12B", "12C", "12D",
	                        "13A", "13B", "13C", "13D",
	                         "13E", "13F", NULL};

	Client::Client (QWidget *parent):QDialog (parent) {
int16_t	i;
	setupUi (this);
	connected	= false;
	connect (connectButton, SIGNAL (clicked (void)),
	         this, SLOT (wantConnect (void)));
	connect (terminateButton, SIGNAL (clicked (void)),
	         this, SLOT (terminate (void)));

	for (i = 0; channelTable [i] != NULL; i ++)
	   channelSelector -> addItem (channelTable [i]);

	buffer	= new RingBuffer<uint8_t> (1024);
	connectionTimer	= new QTimer ();
	connectionTimer	-> setInterval (1000);
	connect (connectionTimer, SIGNAL (timeout (void)),
	         this, SLOT (timerTick (void)));
	connect (channelSelector, SIGNAL (activated (const QString &)),
                 this, SLOT (selectChannel (const QString &)));
	connect (ensembleDisplay, SIGNAL (clicked (QModelIndex)),
                 this, SLOT (selectService (QModelIndex)));
	connect (gainSlider, SIGNAL (valueChanged (int)),
	         this, SLOT (setGain (int)));
	connect (terminateButton, SIGNAL (clicked (void)),
	         this, SLOT (handle_quit (void)));
	state	-> setText ("waiting to start");
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
	basePort	= 8765;
	disconnect (hostLineEdit, SIGNAL (returnPressed (void)),
	            this, SLOT (setConnection (void)));
//
//	The streamer will provide us with the raw data
	streamer. connectToHost (theAddress, basePort + 1);
	if (!streamer. waitForConnected (2000)) {
	   QMessageBox::warning (this, tr ("client"),
	                                   tr ("setting up streamer failed\n"));
	   return;
	}
	writer. connectToHost (theAddress, basePort);
	if (!writer. waitForConnected (2000)) {
	   QMessageBox::warning (this, tr ("client"),
	                                   tr ("setting up writer failed\n"));
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

	while (streamer. bytesAvailable () > 3) {
	d. resize (3);
	streamer. read (d. data (), 3);
	int length = d [1];
	d. resize (3 + length + 1);
	streamer. read (&(d. data () [3]), length + 1);
	buffer -> putDataIntoBuffer (d. data (), d. size ());
	handle ();
	}
}

void	Client::handle (void) {
uint8_t header [3];
std::vector<uint8_t> lBuf;

	buffer -> getDataFromBuffer (header, 3);
	if (header [0] != 0xFF)
	   return;
	int	length	= header [1];
	uint8_t	key	= header [2];
	lBuf. resize (length + 1);
	buffer -> getDataFromBuffer (lBuf. data (), length + 1);
	const char *v	= (char *)(lBuf. data ());
	switch (key) {
	   case Q_SERVICE_NAME:
	      Services << QString (v); 
	      Services. removeDuplicates ();
	      ensemble. setStringList (Services);
	      ensembleDisplay -> setModel (&ensemble);
	      break;

	   case Q_PROGRAM_DATA:
	      programDesc << QString (v);
	      programDesc. removeDuplicates ();
	      programData. setStringList (programDesc);
	      programView	-> setModel (&programData);
	      break;

	   case Q_ENSEMBLE:
	      ensembleName	= QString (v);
	      ensembleLabel	-> setText (ensembleName);
	      dynamicLabel	-> setText ("");
	      break;

	   case Q_TEXT_MESSAGE:
	      dynamicLabel	-> setText (QString (v));
	      break;
	      
	   default:
	      break;
	}
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

int	slength (const char *s) {
int	i;
	for (i = 0; s [i] != 0; i ++);
	return i + 1;
}

void	Client::selectChannel (const QString &s) {
const char *ss = s. toLatin1 (). data ();
QByteArray message;
int length	= 3 + slength (ss);
int	i;

	if (writer. state() == QAbstractSocket::ConnectedState) {
	   message. resize (PACKET_SIZE);
	   for (i = 0; i < length - 2; i ++) {
	      message [3 + i] = ss [i];
	   }
	   message [0] = 0xFF;
	   message [1] = length;
	   message [2] = Q_CHANNEL;
	   writer. write (message);
	}
	ensembleLabel	-> setText ("");
	dynamicLabel	-> setText ("");
	Services = QStringList ();
	ensemble. setStringList (Services);
	ensembleDisplay -> setModel (&ensemble);
	programDesc	= QStringList ();
	programData. setStringList (programDesc);
	programView	-> setModel (&programData);
	dynamicLabel	-> setText ("");
}

void    Client::selectService (QModelIndex s) {
QString currentProgram = ensemble. data (s, Qt::DisplayRole). toString ();
QByteArray message;
const char *ss = currentProgram. toLatin1 (). data ();
int length	= 3 + slength (ss);
int	i;

	if (writer. state() == QAbstractSocket::ConnectedState) {
	   message. resize (PACKET_SIZE);
	   message [0] = 0xFF;
	   message [1] = length;
	   message [2] = Q_SERVICE;
	   for (i = 0; i < length - 2; i ++)
	      message [3 + i] = ss [i];
	   writer. write (message);
	}

	programDesc	= QStringList ();
	programDesc	<< currentProgram;
	programData. setStringList (programDesc);
	programView	-> setModel (&programData);
	dynamicLabel	-> setText ("");
}

void	Client::setGain (int g) {
QByteArray message;
QString gain	= QString::number (g);
const char *ss = gain. toLatin1 (). data ();
int length	= 3 + slength (ss);
int	i;

	if (writer. state() == QAbstractSocket::ConnectedState) {
	   message. resize (PACKET_SIZE);
	   message [0] = 0xFF;
	   message [1] = length;
	   message [2] = Q_GAIN;
	   for (i = 0; i < length - 2; i ++)
	      message [3 + i] = ss [i];
	   writer. write (message);
	}
}

void	Client::handle_quit	(void) {
QByteArray message;
//	if (writer. state() == QAbstractSocket::ConnectedState) {
//	   message. resize (PACKET_SIZE);
//	   message [0] = 0xFF;
//	   message [1] = 3;
//	   message [2] = Q_QUIT;
//	   writer. write (message);
//	}
	accept ();
}

