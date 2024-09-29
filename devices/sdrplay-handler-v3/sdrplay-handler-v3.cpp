#
/*
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the dab library (dab-cmdline)
 *
 *    dab-cmdline is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation version 2 of the License.
 *
 *    dab-cmdline is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab-cmdline if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"sdrplay-handler-v3.h"
#include	"device-exceptions.h"


	sdrplayHandler_v3::sdrplayHandler_v3  (uint32_t	frequency,
	                                       int16_t	ppmCorrection,
	                                       int16_t	GRdB,
	                                       int16_t	lnaState,
	                                       bool	autogain,
	                                       uint16_t	deviceIndex,
	                                       int16_t	antenna,
	                                       bool	X_dump):
	                                          _I_Buffer (4 * 1024 * 1024) {
	this	-> vfoFrequency	= frequency;
	this	-> ppmCorrection	= ppmCorrection;
	this	-> GRdB		= GRdB;
	this	-> lnaState	= lnaState;
	this	-> autogain	= autogain;
	this	-> X_dump	= X_dump;
	failFlag                = false;
//
//	we start the actual handler, but we have to wait until
//	we  know that it is functioning (or not)
        running. store  (false);
        threadHandle            = std::thread (&sdrplayHandler_v3::run, this);
        while (!failFlag && !running. load ())
           usleep (1000);
        if (failFlag) {
           threadHandle. join ();
           throw StartingThreadFailed ();
        }

}

	sdrplayHandler_v3::~sdrplayHandler_v3 () {
	if (X_dump)
	   close_xmlDump ();
	stopReader ();
}

bool	sdrplayHandler_v3::restartReader	(int32_t freq) {
	(void)freq;
	return true;
}

void	sdrplayHandler_v3::stopReader	() {
	if (!running. load ())
	   return;
	running. store (false);
	sleep (1);
	threadHandle. join ();
}

//	The brave old getSamples. For the sdrplay, we get
//	size still in I/Q pairs
//
int32_t	sdrplayHandler_v3::getSamples (std::complex<float> *V, int32_t size) {
	return _I_Buffer. getDataFromBuffer (V, size);
}

int32_t	sdrplayHandler_v3::Samples	() {
	return _I_Buffer. GetRingBufferReadAvailable();
}

void	sdrplayHandler_v3::resetBuffer	() {
	_I_Buffer. FlushRingBuffer	();
}

int16_t	sdrplayHandler_v3::bitDepth	() {
	return nrBits;
}

static
void    StreamACallback (short *xi, short *xq,
                         sdrplay_api_StreamCbParamsT *params,
                         unsigned int numSamples, unsigned int reset,
                         void *cbContext) {
sdrplayHandler_v3 *p	= static_cast<sdrplayHandler_v3 *> (cbContext);
float	denominator	= (float)(p -> denominator);
std::complex<float> localBuf [numSamples];

	(void)params;
	if (reset)
	   return;
	if (!p -> running. load ())
	   return;

	for (int i = 0; i < (int)numSamples; i ++)
	   localBuf [i] = std::complex<float> (((float)xi [i]) / denominator,
	                                         ((float)xq [i]) / denominator);
	int n = (int)(p -> _I_Buffer. GetRingBufferWriteAvailable ());
	if (n >= (int)numSamples)
	   p -> _I_Buffer. putDataIntoBuffer (localBuf, numSamples);
}

static
void	StreamBCallback (short *xi, short *xq,
                         sdrplay_api_StreamCbParamsT *params,
                         unsigned int numSamples, unsigned int reset,
                         void *cbContext) {
	(void)xi; (void)xq; (void)params; (void)cbContext;
        if (reset)
           printf ("sdrplay_api_StreamBCallback: numSamples=%d\n", numSamples);
        return;
}

static
void	EventCallback (sdrplay_api_EventT eventId,
                       sdrplay_api_TunerSelectT tuner,
                       sdrplay_api_EventParamsT *params,
                       void *cbContext) {
sdrplayHandler_v3 *p	= static_cast<sdrplayHandler_v3 *> (cbContext);
	(void)tuner;
	switch (eventId) {
	   case sdrplay_api_GainChange:
	      break;

	   case sdrplay_api_PowerOverloadChange:
	      p -> update_PowerOverload (params);
	      break;

	   default:
	      fprintf (stderr, "event %d\n", eventId);
	      break;
	}
}

void	sdrplayHandler_v3::
	         update_PowerOverload (sdrplay_api_EventParamsT *params) {
	sdrplay_api_Update (chosenDevice -> dev,
	                    chosenDevice -> tuner,
	                    sdrplay_api_Update_Ctrl_OverloadMsgAck,
	                    sdrplay_api_Update_Ext1_None);
	if (params -> powerOverloadParams.powerOverloadChangeType ==
	                                    sdrplay_api_Overload_Detected) {
	}
	else {
	}
}

void	sdrplayHandler_v3::run	() {
sdrplay_api_ErrT        err;
sdrplay_api_DeviceT     devs [6];
float			apiVersion;
uint32_t                ndev;
int			lna_upperBound;

	chosenDevice	= nullptr;
	deviceParams	= nullptr;

	denominator		= 2048.0;		// default
	nrBits			= 12;		// default
	running.  store (false);

//	try to open the API
	err	= sdrplay_api_Open ();
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "sdrplay_api_Open failed %s\n",
	                          sdrplay_api_GetErrorString (err));
	   failFlag	= true;
	   return;
	}

//	Check API versions match
        err = sdrplay_api_ApiVersion (&apiVersion);
        if (err  != sdrplay_api_Success) {
           fprintf (stderr, "sdrplay_api_ApiVersion failed %s\n",
                                     sdrplay_api_GetErrorString (err));
	   failFlag	= true;
	   return;
        }

        if (apiVersion < (SDRPLAY_API_VERSION - 0.1)) {
           fprintf (stderr, "API versions don't match (local=%.2f dll=%.2f)\n",
                                              SDRPLAY_API_VERSION, apiVersion);
	   failFlag	= true;
	   return;
	}

	fprintf (stderr, "api version %f detected\n", apiVersion);
//
//	lock API while device selection is performed
	sdrplay_api_LockDeviceApi ();
	{  int s	= sizeof (devs) / sizeof (sdrplay_api_DeviceT);
	   err	= sdrplay_api_GetDevices (devs, &ndev, s);
	   if (err != sdrplay_api_Success) {
	      fprintf (stderr, "sdrplay_api_GetDevices failed %s\n",
	                         sdrplay_api_GetErrorString (err));
	      goto unlockDevice_closeAPI;
	   }
	}

	if (ndev == 0) {
	   fprintf (stderr, "no valid device found\n");
	   goto unlockDevice_closeAPI;
	}

	fprintf (stderr, "%d devices detected\n", ndev);
	chosenDevice	= &devs [0];
	err	= sdrplay_api_SelectDevice (chosenDevice);
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "sdrplay_api_SelectDevice failed %s\n",
	                         sdrplay_api_GetErrorString (err));
	   goto unlockDevice_closeAPI;
	}
//
//	we have a device, unlock
	sdrplay_api_UnlockDeviceApi ();

	err	= sdrplay_api_DebugEnable (chosenDevice -> dev,
	                                         (sdrplay_api_DbgLvl_t)1);

//	retrieve device parameters, so they can be changed if needed
	err	= sdrplay_api_GetDeviceParams (chosenDevice -> dev,
	                                                     &deviceParams);
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "sdrplay_api_GetDeviceParams failed %s\n",
	                         sdrplay_api_GetErrorString (err));
	   goto closeAPI;
	}

	if (deviceParams == nullptr) {
	   fprintf (stderr, "sdrplay_api_GetDeviceParams return null as par\n");
	   goto closeAPI;
	}
//
//	set the parameter values
	chParams	= deviceParams -> rxChannelA;
	deviceParams	-> devParams -> fsFreq. fsHz	= 2048000;
	chParams	-> tunerParams. bwType = sdrplay_api_BW_1_536;
	chParams	-> tunerParams. ifType = sdrplay_api_IF_Zero;
//
//	these will change:
	chParams	-> tunerParams. rfFreq. rfHz    = vfoFrequency;
	chParams	-> tunerParams. gain.gRdB	= GRdB;
//	chParams	-> tunerParams. gain.LNAstate	= 3;
	chParams	-> tunerParams. gain.LNAstate	= lnaState;
	chParams	-> ctrlParams.agc.enable =
	          autogain ?  sdrplay_api_AGC_100HZ :
	                      sdrplay_api_AGC_DISABLE;
//	assign callback functions
	cbFns. StreamACbFn	= StreamACallback;
	cbFns. StreamBCbFn	= StreamBCallback;
	cbFns. EventCbFn	= EventCallback;

	err	= sdrplay_api_Init (chosenDevice -> dev, &cbFns, this);
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "sdrplay_api_Init failed %s\n",
                                       sdrplay_api_GetErrorString (err));
	   goto unlockDevice_closeAPI;
	}
//
	hwVersion = devs [0]. hwVer;
	switch (hwVersion) {
	   case 1:		// old RSP
	      lna_upperBound	= 3;
	      denominator	= 2048.0;
	      nrBits		= 12;
	      break;
	   case 2:		// RSP II
	      lna_upperBound	= 8;
	      denominator	= 2048.0;
	      nrBits		= 14;
	      break;
	   case 3:		// RSP-DUO
	      lna_upperBound	= 9;
	      denominator	= 2048.0;
	      nrBits		= 12;
	      break;
	   case 4:		// RSP-Dx
	   case 7:		// RSP-Dx_2
	      lna_upperBound	= 26;
	      denominator	= 2048.0;
	      nrBits		= 14;
	      break;
	   case 255:		// RSP-1A
	   case 6:		// RSP-1B
	      lna_upperBound	= 9;
	      denominator	= 4096.0;
	      nrBits		= 14;
	      break;
	}

	if (lnaState >= lna_upperBound)
	   this -> lnaState	= lna_upperBound;

	err = sdrplay_api_Update (chosenDevice -> dev,
	                          chosenDevice -> tuner,
	                          sdrplay_api_Update_Tuner_Frf,
	                          sdrplay_api_Update_Ext1_None);
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "restart: error %s\n",
	                              sdrplay_api_GetErrorString (err));
	   goto closeAPI;

	}
	chParams -> tunerParams. gain. LNAstate = this -> lnaState;
	err = sdrplay_api_Update (chosenDevice -> dev,
	                          chosenDevice -> tuner,
	                          sdrplay_api_Update_Tuner_Gr,
	                          sdrplay_api_Update_Ext1_None);
	if (err != sdrplay_api_Success) {
	   fprintf (stderr, "restart: error in lna setting %s\n",
	                              sdrplay_api_GetErrorString (err));
	   goto closeAPI;

	}
	running. store (true);	// it seems we can do some work

//	Now run the loop "listening" to commands
	while (running. load ()) {
	   usleep (500000);
	}

	running. store (false);
//	normal exit:
	err = sdrplay_api_Uninit	(chosenDevice -> dev);
	if (err != sdrplay_api_Success)
	   fprintf (stderr, "sdrplay_api_Uninit failed %s\n",
	                          sdrplay_api_GetErrorString (err));

	err = sdrplay_api_ReleaseDevice	(chosenDevice);
	if (err != sdrplay_api_Success)
	   fprintf (stderr, "sdrplay_api_ReleaseDevice failed %s\n",
	                          sdrplay_api_GetErrorString (err));

//	sdrplay_api_UnlockDeviceApi	(); ??
        sdrplay_api_Close               ();
	if (err != sdrplay_api_Success)
	   fprintf (stderr, "sdrplay_api_Close failed %s\n",
	                          sdrplay_api_GetErrorString (err));

	return;

unlockDevice_closeAPI:
	sdrplay_api_UnlockDeviceApi	();
closeAPI:
	failFlag		= true;
	sdrplay_api_ReleaseDevice       (chosenDevice);
        sdrplay_api_Close               ();
}

void	sdrplayHandler_v3::setup_xmlDump () {
}

void	sdrplayHandler_v3::close_xmlDump () {
}

