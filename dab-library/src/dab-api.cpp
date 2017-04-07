#
/*
 *    Copyright (C)  2016 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is the implementation part of the DAB-API, it
 *    binds the dab-library to the api.
 *    Many of the ideas as implemented in the DAB-library are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
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
 */

#include	"dab-api.h"
#include	"dab-class.h"
#include	"dab-constants.h"
#include	<stdio.h>
#include	<stdint.h>
#include	"virtual-input.h"
#ifdef	HAVE_DABSTICK
#include	"rtlsdr-handler.h"
#endif
#ifdef	HAVE_SDRPLAY
#include	"sdrplay-handler.h"
#endif
#ifdef	HAVE_AIRSPY
#include	"airspy-handler.h"
#endif

//
//	a global variable and some forward declarations
virtualInput	*setDevice (void);

void	*dab_initialize	(uint8_t	dabMode,  // dab Mode
	                 dabBand	band,	  // Band
	                 cb_audio_t	soundOut, // callback for sound output
	                 cb_data_t	dataOut,  // callback for sound output
	                 int16_t	waitingTime,
	                 cb_system_data_t sysdata,
	                 cb_fib_quality_t fibQuality,
	                 cb_msc_quality_t mscQuality
	                 ) {
virtualInput *inputDevice;

	inputDevice	= setDevice ();
	if (inputDevice == NULL)
	   return NULL;
	return (void *)(new dabClass (inputDevice,
	                              dabMode,
	                              band,
	                              waitingTime,
	                              soundOut,
	                              dataOut,
	                              sysdata,
	                              fibQuality,
	                              mscQuality));
}
	   
void	dab_Gain	(void *handle, uint16_t g) {
	if (((dabClass *)handle) != NULL)
	   ((dabClass *)handle) -> dab_gain (g);
}

void	dab_autoGain	(void *handle, bool b) {
	if (((dabClass *)handle) != NULL)
	   ((dabClass *)handle) -> dab_autogain (b);
}

bool	dab_Channel	(void *handle, std::string name) {
	if (((dabClass *)handle) == NULL)
	   return false;
	((dabClass *)handle) -> dab_stop ();
	return ((dabClass *)handle) -> dab_channel (name);
}

void	dab_run		(void *handle, cb_ensemble_t f) {
	if (((dabClass *)handle) != NULL)
	   ((dabClass *)handle) -> dab_run (f);
}

bool	dab_Service	(void *handle, std::string name, cb_programdata_t t) {
	if (((dabClass *)handle) == NULL)
	   return false;

	if (!((dabClass *)handle) -> dab_running ())
	   return false;


	if (!((dabClass *)handle) -> ensembleArrived ())
	   return false;


	return ((dabClass *)handle) -> dab_service (name, t);
}

bool	dab_Service_Id	(void *handle, int32_t serviceId, cb_programdata_t t) {
	
	if (((dabClass *)handle) == NULL)
	   return false;

	if (!((dabClass *)handle) -> dab_running ())
	   return false;

	if (!((dabClass *)handle) -> ensembleArrived ())
	   return false;


	return ((dabClass *)handle) -> dab_service (serviceId, t);
}
//
void	dab_stop	(void	*handle) {
	if (((dabClass *)handle) == NULL)
	   return;
	if (!((dabClass *)handle) -> dab_running ())
	   return;
	((dabClass *)handle) -> dab_stop ();
}
//
void	dab_exit	(void **handle_p) {
	if (((dabClass *)(*handle_p)) == NULL)
	   return;
	dab_stop (*handle_p);
	delete ((dabClass *)(*handle_p));
	*handle_p	= NULL;
}

int32_t	dab_getSId	(void *handle, std::string name) {
	if (((dabClass *)handle) == NULL)
	   return -1;
	return ((dabClass *)handle) -> dab_getSId (name);
}
	

virtualInput	*setDevice (void) {
bool	success;
virtualInput	*inputDevice;

#ifdef HAVE_AIRSPY
	inputDevice	= new airspyHandler (&success, 80, Mhz (220));
	if (!success) {
	   delete inputDevice;
	   return NULL;
	}
	else 
	   return inputDevice;
#elif defined (HAVE_SDRPLAY)
	inputDevice	= new sdrplayHandler (&success, 60, Mhz (220));
	if (!success) {
	   delete inputDevice;
	   return NULL;
	}
	else 
	   return inputDevice;
#elif defined (HAVE_DABSTICK)
	inputDevice	= new rtlsdrHandler (&success, 75, KHz (220000));
	fprintf (stderr, "input device is rtlsdr handler ? %d\n", success);
	if (!success) {
	   delete inputDevice;
	   return NULL;
	}
	else
	   return inputDevice;
#endif
	fprintf (stderr, "it appeared that you did not select a device, \nyou can run, but you will be connected to a virtual device, only providing zeros\n");
//	and as default option, we have a "virtualInput"
	return  new virtualInput ();
}


