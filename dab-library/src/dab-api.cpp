#
/*
 *    Copyright (C)  2016 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is the implementation part of the dab-api, it
 *	binds the dab-library to the api
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
#include	"dabstick.h"
#endif
#ifdef	HAVE_SDRPLAY
#include	"sdrplay.h"
#endif
#ifdef	HAVE_AIRSPY
#include	"airspy-handler.h"
#endif

//
//	a global variable and some forward declarations
DabParams	dabModeParameters;
void	setModeParameters (uint8_t Mode, DabParams *dabModeParameters);
virtualInput	*setDevice (void);

void	*dab_initialize	(uint8_t     dabMode,	// dab Mode
	                 dabBand     band,	// Band
	                 cb_audio_t  soundOut,	// callback for sound output
	                 cb_data_t   dataOut	// callback for sound output
	                 ) {
virtualInput *inputDevice;

	setModeParameters (dabMode, &dabModeParameters);
	inputDevice	= setDevice ();
	if (inputDevice == NULL)
	   return NULL;
	return (void *)(new dabClass (inputDevice,
	                              &dabModeParameters,
	                              BAND_III,
	                              soundOut,
	                              dataOut));
}
	   
void	dab_Gain	(void *handle, uint16_t g) {
	if (((dabClass *)handle) != NULL)
	   ((dabClass *)handle) -> dab_gain (g);
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


virtualInput	*setDevice (void) {
bool	success;
virtualInput	*inputDevice;

#ifdef HAVE_AIRSPY
	inputDevice	= new airspyHandler (&success, 80, Mhz (220));
	fprintf (stderr, "devic selected\n");
	if (!success) {
	   delete inputDevice;
	   return NULL;
	}
	else 
	   return inputDevice;
#elif defined (HAVE_SDRPLAY)
	inputDevice	= new sdrplay (&success, 60, Mhz (220));
	if (!success) {
	   delete inputDevice;
	   return NULL;
	}
	else 
	   return inputDevice;
#elif defined (HAVE_DABSTICK)
	inputDevice	= new dabStick (&success, 75, KHz (220000));
	if (!success) {
	   delete inputDevice;
	   return NULL;
	}
	else
	   return inputDevice;
#endif
	fprintf (stderr, "it appeared that you did not select a device, \nyou can run, but you will be connected to a virtual device, only providing zeros\n");
//	and as default option, we have a "no device"
	return  new virtualInput ();
}

///	the values for the different Modes:
void	setModeParameters (uint8_t Mode, DabParams *dabModeParameters) {
	if (Mode == 2) {
	   dabModeParameters -> dabMode	= 2;
	   dabModeParameters -> L	= 76;		// blocks per frame
	   dabModeParameters -> K	= 384;		// carriers
	   dabModeParameters -> T_null	= 664;		// null length
	   dabModeParameters -> T_F	= 49152;	// samples per frame
	   dabModeParameters -> T_s	= 638;		// block length
	   dabModeParameters -> T_u	= 512;		// useful part
	   dabModeParameters -> guardLength	= 126;
	   dabModeParameters -> carrierDiff	= 4000;
	} else
	if (Mode == 4) {
	   dabModeParameters -> dabMode		= 4;
	   dabModeParameters -> L		= 76;
	   dabModeParameters -> K		= 768;
	   dabModeParameters -> T_F		= 98304;
	   dabModeParameters -> T_null		= 1328;
	   dabModeParameters -> T_s		= 1276;
	   dabModeParameters -> T_u		= 1024;
	   dabModeParameters -> guardLength	= 252;
	   dabModeParameters -> carrierDiff	= 2000;
	} else 
	if (Mode == 3) {
	   dabModeParameters -> dabMode		= 3;
	   dabModeParameters -> L			= 153;
	   dabModeParameters -> K			= 192;
	   dabModeParameters -> T_F		= 49152;
	   dabModeParameters -> T_null		= 345;
	   dabModeParameters -> T_s		= 319;
	   dabModeParameters -> T_u		= 256;
	   dabModeParameters -> guardLength	= 63;
	   dabModeParameters -> carrierDiff	= 2000;
	} else {	// default = Mode I
	   dabModeParameters -> dabMode		= 1;
	   dabModeParameters -> L			= 76;
	   dabModeParameters -> K			= 1536;
	   dabModeParameters -> T_F		= 196608;
	   dabModeParameters -> T_null		= 2656;
	   dabModeParameters -> T_s		= 2552;
	   dabModeParameters -> T_u		= 2048;
	   dabModeParameters -> guardLength	= 504;
	   dabModeParameters -> carrierDiff	= 1000;
	}
}

