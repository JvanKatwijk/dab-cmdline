#
/*
 *    Copyright (C) 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of simpleDAB
 *
 *    simpleDAB is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    simpleDAB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with simpleDAB; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SIMPLE_DAB__
#define __SIMPLE_DAB__


#include	<QMainWindow>
#include	<QStringList>
#include	<QStringListModel>
#include	<QComboBox>
#include	<QLabel>
#include	<QTimer>
#include	<QMutex>
#include	"dab-api.h"
#include	"ui_dabradio.h"
#include	"ringbuffer.h"
#include	"band-handler.h"

class	deviceHandler;
class	QSettings;
class	virtualInput;
class	audioBase;
class	mscHandler;
class	common_fft;

#include	"ui_technical_data.h"

#ifdef	HAVE_SPECTRUM
class	spectrumhandler;
#endif

/*
 *	The main gui object. It inherits from
 *	QDialog and the generated form
 */
class simpleDab: public QMainWindow,
		      private Ui_dabRadio {
Q_OBJECT
public:
		simpleDab		(QSettings	*,
	                                 QWidget	*parent = NULL);
		~simpleDab		(void);

	QMutex		locker;
private:
	Ui_technical_data	techData;
	QFrame		*dataDisplay;
	deviceHandler	*theDevice;
	QSettings	*dabSettings;
	uint8_t		theBand;
	audioBase	*soundOut;
	RingBuffer<int16_t>	*audioBuffer;
	QStringList	soundChannels;
	QStringListModel	ensemble;
	QStringList	Services;
	QString		ensembleLabel;
	QTimer          displayTimer;
	int32_t         numberofSeconds;
	int32_t		frequency;
	bandHandler	dabBand;
#ifdef	HAVE_SPECTRUM
        spectrumhandler         *spectrumHandler;
#endif
	RingBuffer<std::complex<float>>	*spectrumBuffer;
	RingBuffer<std::complex<float>>	*iqBuffer;

	bool		isStereo;
	void		setStereo		(bool);
	bool		isSynced;
	uint8_t		convert			(QString);
#ifdef	HAVE_SPECTRUM
	void		showSpectrum		(int);
	void		showIQ			(int);
#endif
	void		*theLibrary;
	uint8_t		dabMode;
	int16_t		responseTime;
	int		currentGain;
	QString		currentChannel;
	const char	*get_programm_language_string (int16_t language);
	const char	*get_programm_type_string (int16_t type);
	void		cleanup_GUI		(void);

static	void		audioOut_Handler	(int16_t *, int,
	                                              int, bool, void *);
static	void		dataOut_Handler		(std::string,void *);
static	void		programdata_Handler	(audiodata *, void *);
static	void		fibQuality_Handler	(int16_t, void *);
static	void		mscQuality_Handler	(int16_t, int16_t,
	                                         int16_t, void *);
static	void		sysdata_Handler		(bool, int16_t,
	                                         int32_t, void *);
static	void		ensemblename_Handler	(std::string, int32_t, void *);
static	void		programname_Handler	(std::string, int32_t, void *);
static	void		syncsignal_Handler	(bool, void *);
signals:
	void	do_set_ensembleName	(QString, int);
	void	do_add_programName	(QString);
	void	do_audioOut		(int, bool);
	void	do_set_dynLabel		(QString);
	void	do_set_programData	(int  sa,
                                         int  length,
                                         int  subchId,
                                         int  protLevel,
                                         int  bitRate,
                                         bool     shortForm,
	                                 int	ASCTy,
	                                 int	language,
	                                 int	programType);
	void	do_set_fibQuality	(int);
	void	do_set_mscQuality	(int, int, int);
	void	do_set_sysdata		(bool, int, int);

public slots:
	void	set_ensembleName	(QString, int);
	void	add_programName		(QString);
	void	set_sysdata		(bool, int, int);
	void	set_mscQuality		(int, int, int);
	void	set_fibQuality		(int);
	void	set_programData		(int  sa,
                                         int  length,
                                         int  subchId,
                                         int  protLevel,
                                         int  bitRate,
                                         bool     shortForm,
	                                 int	ASCTy,
	                                 int	language,
	                                 int	programType);
	void	audioOut		(int, bool);
	void	set_dynLabel		(QString);
//	Somehow, these must be connected to the GUI
	void	setStart		(void);
	void	terminateProcess	(void);
	void	set_channelSelect	(const QString &);
	void	updateTimeDisplay	(void);
	void	selectService		(QModelIndex);
	void	set_deviceGain		(int);
	void	set_autoGain		(bool);
	void	set_streamSelector	(int);
	void	do_reset		(void);
};
#endif

