#
/*
 *    Copyright (C) 2013, 2014, 2015, 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of simpleDab
 *
 *    simpleDab is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    simpelDab is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with simpleDab; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	Main program
 */
#include	<QApplication>
#include	<QSettings>
#include	<QDir>
#include	<unistd.h>
#include	"simple-dab.h"

QString	fullPathfor (QString v) {
QString	fileName;

	if (v == QString ("")) 
	   return QString ("/tmp/xxx");

	if (v. at (0) == QChar ('/')) 		// full path specified
	   return v;

	fileName = QDir::homePath ();
	fileName. append ("/");
	fileName. append (v);
	fileName = QDir::toNativeSeparators (fileName);

	if (!fileName. endsWith (".ini"))
	   fileName. append (".ini");

	return fileName;
}

#define	DEFAULT_INI	".simpleDab.ini"

int	main (int argc, char **argv) {
QString	initFileName;
simpleDab	*theInterface;
QSettings	*dabSettings;		// ini file
//
	int	opt;

	while ((opt = getopt (argc, argv, "i:S:TP")) != -1) {
	   switch (opt) {
	      case 'i':
	         initFileName = fullPathfor (QString (optarg));
	         break;

	      case 'S':
	         break;

	      case 'T':
	         break;

	      default:
	         break;
	   }
	}

	if (initFileName == QString (""))
	   initFileName	= fullPathfor (QString (DEFAULT_INI));
	dabSettings =  new QSettings (initFileName, QSettings::IniFormat);

/*
 *	Before we connect control to the gui, we have to
 *	instantiate
 */
	QApplication a (argc, argv);
	theInterface = new simpleDab (dabSettings);
	theInterface -> show ();

#if QT_VERSION >= 0x050600
	QGuiApplication::setAttribute (Qt::AA_EnableHighDpiScaling);
#endif
	a. exec ();
/*
 *	done:
 */
	fflush (stdout);
	fflush (stderr);
	qDebug ("It is done\n");
//	theInterface	-> ~simpleDab ();
}

