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

#ifndef	__BLUETOOTH_CLIENT__
#define	__BLUETOOTH_CLIENT__
#include	"constants.h"
#include	<QDialog>
#include	<QSettings>
#include	<QLabel>
#include	<QMessageBox>
#include	"ui_widget.h"
#include	"ringbuffer.h"
#include	<QStringList>
#include	<QStringListModel>
#include	<QModelIndex>
class	bluetoothHandler;

class   Client:public QDialog, public Ui_widget {
Q_OBJECT
public:
		Client	(QWidget *parent = NULL);
		~Client	(void);

private	slots:
	void		terminate	(void);
	void		selectChannel	(const QString &);
	void		selectService	(QModelIndex);
	void		setGain		(int);
	void		handle_quit	(void);
public	slots:
	void		set_ensembleLabel (const QString &s);
	void		set_serviceName	(const QString &s);
	void		set_textMessage	(const QString &s);
	void		set_programData	(const QString &s);
	void		set_connectionLost (void);
private:

	QStringListModel        ensemble;
	QStringList		Services;
	QString			ensembleName;

	QStringListModel	programData;
	QStringList		programDesc;
	bluetoothHandler	*bluetooth;
};
#endif


