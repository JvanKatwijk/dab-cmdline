#
/*
 *    Copyright (C) 2015, 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the  DAB-cmdline
 *    Many of the ideas as implemented in DAB-cmdline are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are acknowledged.
 *
 *    DAB-cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB-cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB-cmdline; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include	<unistd.h>
#include	<termios.h>
#include	<stdio.h>
#include	<sys/select.h>
#include	<atomic>
#include	"dab-constants.h"
#include	"dab-class.h"

	dabClass::dabClass (virtualInput	*inputDevice,
	                    DabParams		*dabModeParameters,
	                    dabBand		theBand,
	                    int16_t		waitingTime,
	                    cb_audio_t		soundOut,
	                    cb_data_t		dataOut,
	                    cb_system_data_t	systemData,
	                    cb_fib_quality_t	fibQuality,
	                    cb_msc_quality_t	mscQuality) {

	this	-> inputDevice		= inputDevice;
	this	-> dabModeParameters	= dabModeParameters;
	this	-> theBand		= theBand;
	this	-> waitingTime		= waitingTime;
	this	-> soundOut		= soundOut;
	this	-> dataOut		= dataOut;

	deviceGain			= 50;
	autoGain			= false;
	theEnsemble. clearEnsemble ();
	my_ficHandler	= new ficHandler (&theEnsemble, fibQuality);
	my_mscHandler	= new mscHandler (dabModeParameters,
                                          soundOut,
	                                  dataOut,
	                                  mscQuality);

	my_ofdmProcessor = new ofdmProcessor   (inputDevice,
	                                        dabModeParameters,
	                                        systemData,
	                                        my_mscHandler,
	                                        my_ficHandler,
	                                        3, 2);
	run. store (false);
}

	dabClass::~dabClass	(void) {
	if (run. load ()) {	// should not happen
	   run. store (false);
	   threadHandle. join ();
	}

	delete my_ficHandler;
	delete my_mscHandler;
	delete my_ofdmProcessor;
}

void	dabClass::dab_gain	(uint16_t g) {
	deviceGain	= g;
	inputDevice	-> setGain (g);
}

void	dabClass::dab_autogain 	(bool b) {
	autoGain	= b;
	inputDevice	-> setAgc (b);
}

bool	dabClass::dab_running	(void) {
	return run. load ();
}

struct dabFrequencies {
	const char	*key;
	int	fKHz;
};

struct dabFrequencies bandIII_frequencies [] = {
{"5A",	174928},
{"5B",	176640},
{"5C",	178352},
{"5D",	180064},
{"6A",	181936},
{"6B",	183648},
{"6C",	185360},
{"6D",	187072},
{"7A",	188928},
{"7B",	190640},
{"7C",	192352},
{"7D",	194064},
{"8A",	195936},
{"8B",	197648},
{"8C",	199360},
{"8D",	201072},
{"9A",	202928},
{"9B",	204640},
{"9C",	206352},
{"9D",	208064},
{"10A",	209936},
{"10B", 211648},
{"10C", 213360},
{"10D", 215072},
{"11A", 216928},
{"11B",	218640},
{"11C",	220352},
{"11D",	222064},
{"12A",	223936},
{"12B",	225648},
{"12C",	227360},
{"12D",	229072},
{"13A",	230748},
{"13B",	232496},
{"13C",	234208},
{"13D",	235776},
{"13E",	237488},
{"13F",	239200},
{NULL, 0}
};

struct dabFrequencies Lband_frequencies [] = {
{"LA", 1452960},
{"LB", 1454672},
{"LC", 1456384},
{"LD", 1458096},
{"LE", 1459808},
{"LF", 1461520},
{"LG", 1463232},
{"LH", 1464944},
{"LI", 1466656},
{"LJ", 1468368},
{"LK", 1470080},
{"LL", 1471792},
{"LM", 1473504},
{"LN", 1475216},
{"LO", 1476928},
{"LP", 1478640},
{NULL, 0}
};
/**
  */
bool	dabClass::dab_channel (std::string s) {
int16_t	i;
struct dabFrequencies *finger;
int32_t	tunedFrequency;

	tunedFrequency		= 0;
	if (theBand == BAND_III)
	   finger = bandIII_frequencies;
	else
	   finger = Lband_frequencies;

	for (i = 0; finger [i]. key != NULL; i ++) {
	   if (std::string (finger [i]. key) == s) {
	      tunedFrequency	= KHz (finger [i]. fKHz);
	      break;
	   }
	}

	if (tunedFrequency == 0) {
	   fprintf (stderr, "could not find a legal frequency for channel %s\n",
	                                     s. c_str ());
	   return false;
	}

	inputDevice		-> setVFOFrequency (tunedFrequency);
	return true;
}

static
cb_ensemble_t ff;
void	dabClass::dab_run	(cb_ensemble_t f) {
	if (run. load ())
	   return;
	ff	= f;
	threadHandle    = std::thread (&dabClass::run_dab, this, std::ref(ff));
}

void	dabClass::run_dab	(cb_ensemble_t h) {

	if (run. load ())
	   return;

//	Note: the ofdmProcessor will restart the inputDevice
	my_ofdmProcessor	-> start ();
	inputDevice		-> setGain (deviceGain);
	inputDevice		-> setAgc (autoGain);
	run. store (true);
	sleep (waitingTime);	// give everyone the opportunity to do domething

	fprintf (stderr, "going to tell the world\n");
	h (theEnsemble. data (), theEnsemble. ensembleExists ());
	fprintf (stderr, "we told the world\n");
	while (run. load ())
	   usleep (10000);
//
//	we started the ofdmprocessor, so we also stop it here
	my_ofdmProcessor	-> stop ();
}

bool	dabClass::dab_service (int32_t serviceId, cb_programdata_t t) {
	return dab_service (my_ficHandler -> nameFor (serviceId), t);
}

bool	dabClass::dab_service (std::string aname, cb_programdata_t t) {
audiodata d;
std::string name	= theEnsemble. findService (aname);

	switch (my_ficHandler -> kindofService (name)) {
	   case AUDIO_SERVICE:
	     my_ficHandler	-> dataforAudioService (name, &d);
	     if (d. defined) {
	        my_mscHandler	-> set_audioChannel (&d);
	        fprintf (stderr, "selected %s\n", name. c_str ());
	        if (t != NULL) 
	          t (d. startAddr, d. length, d. subchId, d. bitRate, d. protLevel);
	        return true;
	     }
	     else {
	        fprintf (stderr, "Insufficient data to execute service %s",
	                                           name. c_str ());
	        return false;
	     }
	     break;
//
//	For the command line version, we do not have a data service
	   case PACKET_SERVICE:
	      fprintf (stderr,
	               "Data services not supported in this version\n");
	      return false;

	   default:
	      fprintf (stderr,
	               "kind of service %s unclear, fatal\n", name. c_str ());
	      return false;
	}
	return false;
}

void	dabClass::dab_stop (void) {
	if (run. load ()) {
	   run. store (false);
	   threadHandle. join ();
	}
}

bool	dabClass::ensembleArrived (void) {
	return theEnsemble. ensembleExists  ();
}

