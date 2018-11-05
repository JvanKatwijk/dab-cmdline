#
/*
 *    Copyright (C) 2015-2018
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
 */

#include	<QSettings>
#include	<QLabel>
#include	<QMessageBox>
#include	"protocol.h"
#include	"client.h"
#include	"bluetooth-handler.h"
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
	connect (terminateButton, SIGNAL (clicked (void)),
	         this, SLOT (terminate (void)));

	for (i = 0; channelTable [i] != NULL; i ++)
	   channelSelector -> addItem (channelTable [i]);

	try {
	   bluetooth	= new bluetoothHandler ();
	} catch (int e) {
	   fprintf (stderr, "Could not connect to a server\n");
	   exit (21);
	}

	connect (channelSelector, SIGNAL (activated (const QString &)),
                 this, SLOT (selectChannel (const QString &)));
	connect (ensembleDisplay, SIGNAL (clicked (QModelIndex)),
                 this, SLOT (selectService (QModelIndex)));
	connect (ifgainReduction, SIGNAL (valueChanged (int)),
	         this, SLOT (set_ifgainReduction (int)));
	connect (lnaState, SIGNAL (valueChanged (int)),
	         this, SLOT (set_lnaState (int)));
	connect (autogainButton, SIGNAL (stateChanged (int)),
	         this, SLOT (set_autoGain (int)));
//
//	the following signals arrive from the bluetooth handling
	connect (bluetooth, SIGNAL (ensembleLabel (const QString &)),
	         this, SLOT (set_ensembleLabel (const QString &)));
	connect (bluetooth, SIGNAL (serviceName (const QString &)),
	         this, SLOT (set_serviceName (const QString &)));
	connect (bluetooth, SIGNAL (textMessage (const QString &)),
	         this, SLOT (set_textMessage (const QString &)));
	connect (bluetooth, SIGNAL (programData (const QString &)),
	         this, SLOT (set_programData (const QString &)));
	connect (bluetooth, SIGNAL (connectionLost (void)),
	         this, SLOT (set_connectionLost (void)));
}
//

	Client::~Client	(void) {
}

void	Client::selectChannel (const QString &s) {
const char *ss = s. toLatin1 (). data ();
int length	= strlen (ss);
char	message [length + 4];
int	i;

	for (i = 0; i < length; i ++) {
	  message [3 + i] = ss [i];
	}
	message [3 + length] = 0;
	fprintf (stderr, "selected %s\n", &(message [3]));
	message [0] = Q_CHANNEL;
	message [1] = (length >> 8) & 0xFF;
	message [2] = length & 0xFF;
	bluetooth -> write (message, length + 3 + 1);

	ensembleLabel	-> setText ("");
	dynamicLabel	-> setText ("");
	Services	= QStringList ();
	ensemble. setStringList (Services);
	ensembleDisplay -> setModel (&ensemble);
	programDesc	= QStringList ();
	programData. setStringList (programDesc);
	programView	-> setModel (&programData);
	dynamicLabel	-> setText ("");
}

void    Client::selectService (QModelIndex s) {
QString currentProgram = ensemble. data (s, Qt::DisplayRole). toString ();
const char *ss 	= currentProgram. toLatin1 (). data ();
int length	= strlen (ss);
char	message [length + 4];
int	i;

	for (i = 0; i < length; i ++) {
	   message [3 + i] = ss [i];
	   fprintf (stderr, "%c ", ss [i]);
	}
	message [length + 3] = 0;
	fprintf (stderr, "\n");
	message [0] = Q_SERVICE;
	message [1] = (length >> 8) & 0xFF;
	message [2] = length & 0xFF;
	bluetooth -> write (message, length + 3 + 1);

	programDesc	= QStringList ();
	programDesc	<< currentProgram;
	programData. setStringList (programDesc);
	programView	-> setModel (&programData);
	dynamicLabel	-> setText ("");
}

void	Client::handle_quit	(void) {
	accept ();
}

void	Client::set_ensembleLabel (const QString &s) {
	ensembleLabel	-> setText (s);
	dynamicLabel	-> setText (" ");
}

void	Client::set_serviceName (const QString &s) {
	Services << QString (s);
        Services. removeDuplicates ();
        ensemble. setStringList (Services);
        ensembleDisplay -> setModel (&ensemble);
}

void	Client::set_textMessage (const QString &s) {
	dynamicLabel	-> setText (s);
}

void	Client::set_programData (const QString &s) {
	programDesc << QString (s);
        programDesc. removeDuplicates ();
        programData. setStringList (programDesc);
	programView       -> setModel (&programData);
}

void	Client::set_connectionLost (void) {
}

void	Client::terminate	(void) {
}

void	Client::set_ifgainReduction (int v) {
QString val	= QString::number (v);
const char *ss	= val. toLatin1 (). data ();
int length	= strlen (ss);
char	message [length + 4];
int	i;

	for (i = 0; i < length; i ++)
	   message [3 + i] = ss [i];
	message [3 + length] = 0;
	message [0] = Q_IF_GAIN_REDUCTION;
	message [1] = (length >> 8) & 0xFF;
	message [2] = length & 0xFF;
	bluetooth -> write (message, length + 4);
}

void	Client::set_lnaState (int v) {
QString val	= QString::number (v);
const char *ss	= val. toLatin1 (). data ();
int length	= strlen (ss);
char	message [length + 4];
int	i;

	for (i = 0; i < length; i ++)
	   message [3 + i] = ss [i];
	message [3 + length] = 0;
	message [0] = Q_LNA_STATE;
	message [1] = (length >> 8) & 0xFF;
	message [2] = length & 0xFF;
	bluetooth -> write (message, length + 4);
}

void	Client::set_autoGain (int state) {
QString val     = QString::number (state);
const char *ss  = val. toLatin1 (). data ();
int length      = strlen (ss);
char    message [length + 4];
int     i;

        for (i = 0; i < length; i ++)
           message [3 + i] = ss [i];
        message [3 + length] = 0;
        message [0] = Q_AUTOGAIN;
        message [1] = (length >> 8) & 0xFF;
        message [2] = length & 0xFF;
        bluetooth -> write (message, length + 4);
}

void	Client::set_resetButton (void) {
char    message [4];
int     i;

        message [3 + 1] = 0;
        message [0] = Q_RESET;
        message [1] = 0;
        message [2] = 0;
        bluetooth -> write (message, 0);
}

