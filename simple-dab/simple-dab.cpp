#
/*
 *    Copyright (C) 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of simpleDab
 *    simpleDab is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    simpleDab is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with simpleDab; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	<QSettings>
#include	<QMessageBox>
#include	<QDebug>
#include	<QStringList>
#include	<QStringListModel>
#include	<QDir>
#include        <fstream>
#include        <iostream>
#include	<numeric>
#include	<unistd.h>
#include	<vector>
#include	"dab-api.h"
#include	"simple-dab.h"
#include	"audiosink.h"
#include	"ui_technical_data.h"
#ifdef	HAVE_SPECTRUM
#include	"spectrum-handler.h"
#endif
#ifdef	HAVE_RTLSDR
#include	"rtlsdr-handler.h"
#elif	HAVE_AIRSPY
#include	"airspy-handler.h"
#else
#include	"sdrplay-handler.h"
#endif

std::vector<size_t> get_cpu_times() {
	std::ifstream proc_stat("/proc/stat");
	proc_stat.ignore(5, ' '); // Skip the 'cpu' prefix.
	std::vector<size_t> times;
	for (size_t time; proc_stat >> time; times.push_back(time));
	return times;
}
 
bool get_cpu_times (size_t &idle_time, size_t &total_time) {
	const std::vector<size_t> cpu_times = get_cpu_times();
	if (cpu_times.size () < 4)
	   return false;
	idle_time = cpu_times [3];
	total_time = std::accumulate (cpu_times. begin (), cpu_times. end (), 0);
	return true;
}

const char * Band_III_table [] =
	{"5A",  "5B",  "5C",  "5D",  "6A",  "6B",  "6C",  "6D", 
	 "7A",  "7B",  "7C",  "7D",  "8A",  "8B",  "8C",  "8D",
	 "9A",  "9B",  "9C",  "9D", "10A", "10B", "10C", "10D",
	"11A", "11B", "11C", "11D", "12A", "12B", "12C", "12D",
	"13A", "13B", "13C", "13D", "13E", "13F", NULL
	};

const char * L_Band_table [] =
	{"LA", "LB", "LC", "LD", "LE", "LF", "LG", "LH", NULL};
//
	simpleDab::simpleDab (QSettings	*Si,
	                      QWidget	*parent): QMainWindow (parent) {

int	i;
const char	**t;

	dabSettings		= Si;
// 	the setup for the generated part of the ui
	setupUi (this);
//
//	The datadisplay is separate
	dataDisplay	= new QFrame (NULL);
	techData. setupUi (dataDisplay);
	dataDisplay -> show ();
/*
 */
	audioBuffer		= new RingBuffer<int16_t>(32768);
	soundOut		= new audioSink		(2, audioBuffer);
	((audioSink *)soundOut)	-> setupChannels (streamoutSelector);
	bool err;
	QString h		=
	          dabSettings -> value ("soundchannel", "default"). toString ();
	int k	= streamoutSelector -> findText (h);
	if (k != -1) {
	   streamoutSelector -> setCurrentIndex (k);
	   err = !((audioSink *)soundOut) -> selectDevice (k);
	}

	if ((k == -1) || err)
	   ((audioSink *)soundOut)	-> selectDefaultDevice ();
	connect (streamoutSelector, SIGNAL (activated (int)),
	         this,  SLOT (set_streamSelector (int)));

	h	=
	         dabSettings -> value ("Band", "Band III"). toString ();
	theBand = (h == "L Band") ? L_BAND : BAND_III;
	dabMode		= 1;

#ifdef	HAVE_SPECTRUM
	spectrumBuffer	= new RingBuffer<std::complex<float>> (32 * 32768);
	iqBuffer	= new RingBuffer<std::complex<float>> (32768);
	spectrumHandler	= new spectrumhandler (this,
	                                       dabSettings,
	                                       spectrumBuffer,
	                                       iqBuffer);
	spectrumHandler	-> show ();
#else
	spectrumBuffer	= NULL;
	iqBuffer	= NULL;
#endif
//	main function: to create the library with the right settings
//	and the callbacks to provide status info on the screen

	if (theBand == BAND_III) 
	   t = Band_III_table;
	else
	   t = L_Band_table;

	for (i = 0; t [i] != NULL; i ++)
	   channelSelector -> insertItem (i, t [i], QVariant (i));

//	default settings
	currentGain		= 
	       dabSettings -> value ("gainsetting", 50). toInt ();
	gainSlider	-> setValue (currentGain);
//
//	set the channel
	h		=
	          dabSettings -> value ("channel", "11C"). toString ();
	k		=
	          channelSelector -> findText (h);
	if (k != -1) {
	   channelSelector -> setCurrentIndex (k);
	}

	currentChannel		= channelSelector -> currentText ();
	std::string ss		= std::string (currentChannel. toLatin1 (). data ());
//	and finally:
	frequency	= dabBand. Frequency (theBand, 
                                              std::string (currentChannel. toLatin1 (). data ()));
	try {
#ifdef	HAVE_RTLSDR
	   theDevice	= new rtlsdrHandler (frequency,
	                                     0,
	                                     gainSlider -> value (),
	                                     true, 0);
#elif	HAVE_AIRSPY
	   theDevice	= new airspyHandler (frequency,
	                                     3,
	                                     gainSlider -> value ());
#else
	   theDevice	= new sdrplayHandler (frequency,
	                                      3,
	                                      gainSlider -> value (),
	                                      true, 0, 0);
#endif
	}
	catch (int e) {
	   fprintf (stderr, "allocating device failed, fatal\n");
	   exit (2);
	}

	if (!theDevice -> has_autogain ())
	   autogain_switch -> hide ();
	   
//	Note that the callbacks are plain static functions
	theLibrary	= dabInit (theDevice,
	                           dabMode,		// Mode for now
	                           spectrumBuffer,
	                           iqBuffer,
	                           syncsignal_Handler,
	                           sysdata_Handler,
	                           ensemblename_Handler,
	                           programname_Handler,
	                           fibQuality_Handler,
	                           audioOut_Handler,
	                           dataOut_Handler,
	                           NULL,
	                           programdata_Handler,
	                           mscQuality_Handler,
	                           NULL,	// no mot slides
	                           this		// context, userData
	                          );
	
	if (theLibrary == NULL) {
	   fprintf (stderr, "Sorry, could not initialize the library, fatal\n");
	   close ();
	   exit (101);
	}


//	Handling the GUI buttons and sliders
	connect (ensembleDisplay, SIGNAL (clicked (QModelIndex)),
                      this, SLOT (selectService (QModelIndex)));
        connect (startButton, SIGNAL (clicked (void)),
                      this, SLOT (setStart (void)));
        connect (quitButton, SIGNAL (clicked ()),
                      this, SLOT (terminateProcess (void)));
        connect (channelSelector, SIGNAL (activated (const QString &)),
                      this, SLOT (set_channelSelect (const QString &)));
        connect (resetButton, SIGNAL (clicked (void)),
                      this, SLOT (do_reset (void)));
	connect (gainSlider, SIGNAL (valueChanged (int)),
	              this, SLOT (set_deviceGain (int)));
	connect (autogain_switch, SIGNAL (clicked (bool)),
	              this, SLOT (set_autoGain (bool)));
//
//	A little unwieldy, but inevitable since the externally
//	called callback functions have to be connected to decent
//	Qt signals. The approach is: the static callback functions
//	call ("emit") signals, that are bound to slots
//	that do the actual work
	connect (this, SIGNAL (do_add_programName (QString)),
	         this, SLOT   (add_programName   (QString)));
	connect (this, SIGNAL (do_set_ensembleName (QString, int)),
	         this, SLOT   (set_ensembleName (QString, int)));
	connect (this, SIGNAL (do_audioOut (int, bool)),
	         this, SLOT   (audioOut (int, bool)));
	connect (this, SIGNAL (do_set_dynLabel (QString)),
	         this, SLOT   (set_dynLabel (QString)));
	connect (this, SIGNAL (do_set_programData (int, int, int,
	                                           int, int, bool,
	                                           int, int, int)),
	         this, SLOT   (set_programData    (int, int, int,
	                                           int, int, bool,
	                                           int, int, int)));
	connect (this, SIGNAL (do_set_fibQuality (int)),
	         this, SLOT   (set_fibQuality    (int)));
	connect (this, SIGNAL (do_set_mscQuality (int, int, int)),
	         this, SLOT   (set_mscQuality    (int, int, int)));
	connect (this, SIGNAL (do_set_sysdata    (bool, int, int)),
	         this, SLOT   (set_sysdata       (bool, int, int)));

/**
  *	The only timer we use is for displaying the running time.
  *	The number of seconds passed is kept in numberofSeconds
  */	
	versionName	-> setText ("simpleDab 0.55");
	numberofSeconds		= 0;
	displayTimer. setInterval (1000);
	connect (&displayTimer, SIGNAL (timeout (void)),
	         this, SLOT (updateTimeDisplay (void)));
	displayTimer. start (1000);
	isStereo	= true;
	setStereo (false);	// trick to set the color right
}

	simpleDab::~simpleDab		(void) {
	delete theDevice;
}

void	simpleDab::set_channelSelect (const QString &s) {
std::string ss;
	if (theLibrary == NULL)
	   return;
	cleanup_GUI ();
//	reset (theLibrary);
//
//	The service name as C std::string
	ss			= std::string (s. toLatin1 (). data ());
//	set the channel
	frequency	= dabBand. Frequency (theBand, ss);
	theDevice	-> setVFOFrequency (frequency);
//	and run again
}

void	simpleDab::selectService (QModelIndex s) {
QString a = ensemble. data (s, Qt::DisplayRole). toString ();
std::string ss = std::string (a. toLatin1 (). data ());
	dabService (ss.c_str(), theLibrary);
	techData. programName  -> setText (a);
}

void	simpleDab::set_deviceGain	(int g) {
	if (theLibrary == NULL)
	   return;
	currentGain	= g;
	theDevice	-> setGain (g);
}

void	simpleDab::set_autoGain		(bool b) {
	if (theLibrary == NULL)
	   return;
	(void)b;
}
//
//	setStart obviously only makes sense when not running already
void	simpleDab::setStart		(void) {
	if (theLibrary == NULL)
	   return;
	theDevice	-> restartReader ();
	dabStartProcessing (theLibrary);
}
//
//
void	simpleDab::terminateProcess	(void) {
	if (theLibrary == NULL)
	   return;
	theDevice	-> stopReader ();
	fprintf (stderr, "the device is stopped\n");
	dabStop (theLibrary);
	fprintf (stderr, "the library stopped\n");
	dabSettings	->
	           setValue ("channel", channelSelector -> currentText ());
	dabSettings	->
	           setValue ("gainsetting", gainSlider -> value ());
	dabSettings	->
	           setValue ("soundchannel", streamoutSelector -> currentText ());
	dabSettings	-> sync ();
	dataDisplay     ->  hide ();
        delete dataDisplay;
	fprintf (stderr, "the dataDisplay disappeared\n");
#ifdef	HAVE_SPECTRUM
	spectrumHandler -> hide ();
	delete  spectrumHandler;
	delete  spectrumBuffer;
	delete  iqBuffer;
#endif
	close ();
	fprintf (stderr, "close executed\n");
}

void	simpleDab::do_reset		(void) {
	if (theLibrary == NULL)
	   return;
	cleanup_GUI ();
	dabReset (theLibrary);
}

void    simpleDab:: set_streamSelector (int k) {
#ifndef TCP_STREAMER
        ((audioSink *)(soundOut)) -> selectDevice (k);
#else
        (void)k;
#endif
}
//
///////////////////////////////////////////////////////////////////////////
//
//	Handling callbacks, we hve to "translate" then into 
//	regular Qt signals

void	simpleDab::audioOut_Handler	(int16_t *buffer,
	                                 int size,
	                                 int rate,
	                                 bool stereo,
	                                 void *context) {
simpleDab *ctx	= reinterpret_cast <simpleDab *>(context);
	ctx -> locker. lock ();
	ctx -> audioBuffer	-> putDataIntoBuffer (buffer, size);
	emit ctx -> do_audioOut (rate, stereo);
	ctx -> locker. unlock ();
}

void	simpleDab::dataOut_Handler	(std::string text, void *context) {
simpleDab *ctx	= reinterpret_cast <simpleDab *>(context);
	ctx -> locker. lock ();
	emit ctx -> do_set_dynLabel (QString::fromStdString (text));
	ctx -> locker. unlock ();
}

void	simpleDab::programdata_Handler (audiodata *d, void *context) {
simpleDab *ctx	= reinterpret_cast <simpleDab *>(context);
	ctx -> locker. lock ();
	emit ctx -> do_set_programData (d -> startAddr,
	                                d -> length,
	                                d -> subchId,
	                                d -> protLevel,
	                                d -> bitRate,
	                                d -> shortForm,
	                                d -> ASCTy,
	                                d -> language,
	                                d -> programType);
	ctx -> locker. unlock ();
}

void	simpleDab::fibQuality_Handler (int16_t q, void	*context) {
simpleDab *ctx	= reinterpret_cast <simpleDab *>(context);
	ctx -> locker. lock ();
	emit ctx -> do_set_fibQuality (q);
	ctx -> locker. unlock ();
}

void	simpleDab::mscQuality_Handler (int16_t frameQuality,
	                              int16_t rsQuality,
	                              int16_t aacQuality,
	                              void   *context
	                              ) {
simpleDab *ctx	= reinterpret_cast <simpleDab *>(context);
	ctx -> locker. lock ();
	emit ctx -> do_set_mscQuality (frameQuality, rsQuality, aacQuality);
	ctx -> locker. unlock ();
}
//
//	we know that if/when this callback is called,
//	we can also look into the spectrum data
void	simpleDab::sysdata_Handler    (bool	ts,	// timesync,
	                               int16_t	snr,	// signal noise ratio
	                               int32_t	offs,	// frequency offset
	                               void 	*context
	                              ) {
simpleDab *ctx	= reinterpret_cast <simpleDab *>(context);
	ctx -> locker. lock ();
	emit ctx -> do_set_sysdata (ts, snr, offs);
	ctx -> locker. unlock ();
}

void	simpleDab::syncsignal_Handler (bool b, void *userData) {
simpleDab *ctx	= reinterpret_cast <simpleDab *>(userData);
	ctx -> locker. lock ();
	if (!b)
	   fprintf (stderr,
	          "better select another channel, this one fails\n");
	ctx -> locker. unlock ();
}

void	simpleDab::ensemblename_Handler (std::string name,
	                                 int32_t Id,  void *context) {
simpleDab *ctx	= reinterpret_cast <simpleDab *>(context);
	ctx -> locker. lock ();
	ctx	-> do_set_ensembleName (QString::fromStdString (name), Id);
	ctx -> locker. unlock ();
}

void	simpleDab::programname_Handler (std::string name,
	                                int32_t SId, void *userData) {
simpleDab *ctx	= reinterpret_cast <simpleDab *>(userData);
	ctx -> locker. lock ();
	ctx	-> do_add_programName (QString::fromStdString (name));
	ctx -> locker. unlock ();
}

void	simpleDab::add_programName (QString s) {
	Services << s;
	ensemble. setStringList (Services);
	ensembleDisplay -> setModel (&ensemble);
}

void	simpleDab::set_ensembleName (QString s, int Id) {
	ensembleName		-> setText (s);
	ensembleId		-> display (Id);
	techData. ensemble	-> setText (s);
	techData. frequency	-> display (frequency / 1000000.0);
}

void	simpleDab::audioOut (int rate, bool stereo) {
	soundOut	-> audioOut (rate);
	setStereo (stereo);
}

void    simpleDab::setStereo       (bool s) {
	if (isStereo == s)
	   return;

	isStereo	= s;
        if (s)
           stereoLabel ->
                       setStyleSheet ("QLabel {background-color : green}");

        else
           stereoLabel ->
                       setStyleSheet ("QLabel {background-color : red}");
}

void	simpleDab::set_dynLabel (QString s) {
	dynamicLabel -> setText (s);
}

void	simpleDab::set_programData (int		sa,
                                    int  length,
                                    int  subchId,
                                    int  protLevel,
                                    int  bitRate,
                                    bool     shortForm,
	                            int	ASCTy,
	                            int	language,
	                            int programType) {
	techData. bitrateDisplay	-> display (bitRate);
	techData. startAddressDisplay	-> display (sa);
	techData. lengthDisplay		-> display (length);
	techData. subChIdDisplay	-> display (subchId);

	uint16_t h = protLevel;
	QString protL;
	if (!shortForm) {
	   protL = "EEP ";
	   if ((h & (1 << 2)) == 0) 
	      protL. append ("A ");
	   else
	      protL. append ("B ");
	   h = (h & 03) + 1;
	   protL. append (QString::number (h));
	}
	else  {
	   h = h & 03;
	   protL = "UEP ";
	   protL. append (QString::number (h));
	}
	techData. uepField -> setText (protL);
	techData. protectionlevelDisplay -> display (h);
	techData. ASCTy -> setText (ASCTy == 077 ? "DAB+" : "DAB");
	techData. language ->
	        setText (get_programm_language_string (language));
	techData. programType ->
	        setText (get_programm_type_string (programType));
}

void	simpleDab::set_fibQuality (int q) {
	techData. ficError_display -> setValue (q);
}

void	simpleDab::set_mscQuality (int frameQuality,
	                           int rsQuality, int aacQuality) {
	techData. frameError_display	-> setValue (frameQuality);
	techData. rsError_display	-> setValue (rsQuality);
	techData. aacError_display	-> setValue (aacQuality);
}

void	simpleDab::set_sysdata (bool ts, int snr, int offs) {
	snrDisplay	-> display (snr);
	offsetDisplay	-> display (offs);
	if (isSynced != ts) {
	   isSynced = ts;
	   switch (isSynced) {
	      case true:
	         syncedLabel -> 
	               setStyleSheet ("QLabel {background-color : green}");
	         break;

	      default:
	         syncedLabel ->
	               setStyleSheet ("QLabel {background-color : red}");
	         break;
	   }
	}
#ifdef	HAVE_SPECTRUM
	spectrumHandler	-> showSpectrum (frequency);
	spectrumHandler	-> showIQ ();
#endif
}

////////////////////////////////////////////////////////////////////////////

static 
const char *table12 [] = {
"None",
"News",
"Current affairs",
"Information",
"Sport",
"Education",
"Drama",
"Arts",
"Science",
"Talk",
"Pop music",
"Rock music",
"Easy listening",
"Light classical",
"Classical music",
"Other music",
"Wheather",
"Finance",
"Children\'s",
"Factual",
"Religion",
"Phone in",
"Travel",
"Leisure",
"Jazz and Blues",
"Country music",
"National music",
"Oldies music",
"Folk music",
"entry 29 not used",
"entry 30 not used",
"entry 31 not used"
};

const char *simpleDab::get_programm_type_string (int16_t type) {
	if (type > 0x40) {
	   fprintf (stderr, "GUI: program type wrong (%d)\n", type);
	   return (table12 [0]);
	}
	if (type < 0)
	   return " ";

	return table12 [type];
}

static
const char *table9 [] = {
"unknown language",
"Albanian",
"Breton",
"Catalan",
"Croatian",
"Welsh",
"Czech",
"Danish",
"German",
"English",
"Spanish",
"Esperanto",
"Estonian",
"Basque",
"Faroese",
"French",
"Frisian",
"Irish",
"Gaelic",
"Galician",
"Icelandic",
"Italian",
"Lappish",
"Latin",
"Latvian",
"Luxembourgian",
"Lithuanian",
"Hungarian",
"Maltese",
"Dutch",
"Norwegian",
"Occitan",
"Polish",
"Postuguese",
"Romanian",
"Romansh",
"Serbian",
"Slovak",
"Slovene",
"Finnish",
"Swedish",
"Tuskish",
"Flemish",
"Walloon"
};

const char *simpleDab::get_programm_language_string (int16_t language) {
	if (language > 43) {
	   fprintf (stderr, "GUI: wrong language (%d)\n", language);
	   return table9 [0];
	}
	if (language < 0)
	   return " ";
	return table9 [language];
}


static size_t previous_idle_time	= 0;
static size_t previous_total_time	= 0;

void	simpleDab::updateTimeDisplay (void) {
	numberofSeconds ++;
	int16_t	numberHours	= numberofSeconds / 3600;
	int16_t	numberMinutes	= (numberofSeconds / 60) % 60;
	QString text = QString ("runtime ");
	text. append (QString::number (numberHours));
	text. append (" hr, ");
	text. append (QString::number (numberMinutes));
	text. append (" min");
	timeDisplay	-> setText (text);
	if ((numberofSeconds % 2) == 0) {
	   size_t idle_time, total_time;
	   get_cpu_times (idle_time, total_time);
	   const float idle_time_delta = idle_time - previous_idle_time;
           const float total_time_delta = total_time - previous_total_time;
           const float utilization = 100.0 * (1.0 - idle_time_delta / total_time_delta);
	   techData. cpuMonitor -> display (utilization);
           previous_idle_time = idle_time;
           previous_total_time = total_time;
	}
}

void	simpleDab::cleanup_GUI	(void) {
	Services		= QStringList ();
        ensemble. setStringList (Services);
        ensembleDisplay         -> setModel (&ensemble);

        ensembleLabel           = QString ();
        ensembleName            -> setText (ensembleLabel);
        dynamicLabel            -> setText ("");

//      Then the various displayed items
        ensembleName            -> setText ("   ");
        techData. frameError_display    -> setValue (0);
        techData. rsError_display       -> setValue (0);
        techData. aacError_display      -> setValue (0);
        techData. ficError_display      -> setValue (0);
        techData. ensemble              -> setText (QString (""));
        techData. programName           -> setText (QString (""));
        techData. frequency             -> display (0);
        techData. bitrateDisplay        -> display (0);
        techData. startAddressDisplay   -> display (0);
        techData. lengthDisplay         -> display (0);
        techData. subChIdDisplay        -> display (0);
	techData. protectionlevelDisplay -> display (0);
        techData. uepField              -> setText (QString (""));
        techData. ASCTy                 -> setText (QString (""));
        techData. language              -> setText (QString (""));
        techData. programType           -> setText (QString (""));
}

