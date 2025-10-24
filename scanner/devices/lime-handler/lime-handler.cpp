#
/*
 *    Copyright (C) 2014 .. 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the dab library
 *
 *    dab library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dab library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dab library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"lime-handler.h"
#include	<unistd.h>
#include  "device-exceptions.h"

#define	FIFO_SIZE	32768
static
float localBuffer [4 * FIFO_SIZE];
lms_info_str_t limedevices [10];

	limeHandler::limeHandler (int32_t frequency,
	                          int16_t gain,
	                          std::string antenna) {
	this	-> frequency	= frequency;
	this	-> gain		= gain;

	int ndevs	= LMS_GetDeviceList (limedevices);
	if (ndevs == 0) {	// no devices found
	   throw DeviceNotFound();
	}

	for (int i = 0; i < ndevs; i ++)
	   DEBUG_PRINT ("device %s\n", limedevices [i]);

	int res		= LMS_Open (&theDevice, NULL, NULL);
	if (res < 0) {	// some error
	   throw OpeningFailed();
	}

	res		= LMS_Init (theDevice);
	if (res < 0) {	// some error
	   LMS_Close (&theDevice);
	   throw InitFailed();
	}

	res		= LMS_GetNumChannels (theDevice, LMS_CH_RX);
	if (res < 0) {	// some error
	   LMS_Close (&theDevice);
	   throw GetChannelsFailed();
	}

	DEBUG_PRINT ("device %s supports %d channels\n",
	                            limedevices [0], res);
	res		= LMS_EnableChannel (theDevice, LMS_CH_RX, 0, true);
	if (res < 0) {	// some error
	   LMS_Close (theDevice);
	   throw OpeningChannelFailed("RX","0");
	}

	res	= LMS_SetSampleRate (theDevice, 2048000.0, 0);
	if (res < 0) {
	   LMS_Close (theDevice);
	   throw SampleRateFailed();
	}

	float_type host_Hz, rf_Hz;
	res	= LMS_GetSampleRate (theDevice, LMS_CH_RX, 0,
	                               &host_Hz, &rf_Hz);

	DEBUG_PRINT ("samplerate = %f %f\n", (float)host_Hz, (float)rf_Hz);

	res		= LMS_GetAntennaList (theDevice,
	                                      LMS_CH_RX, 0, antennas);
	DEBUG_PRINT ("Antennes found: ");
	for (int i = 0; i < res; i ++)
	   DEBUG_PRINT ("%s ", antennas [i]);
	DEBUG_PRINT ("\n");
	for (int i = 0; i < res; i ++)
	   if (std::string (antennas [i]). compare (antenna) == 0) {
	      DEBUG_PRINT ("Auto selected (index = %d)\n", i);
	      res	= LMS_SetAntenna (theDevice, LMS_CH_RX, 0, i);
	   }

//	default frequency
	res		= LMS_SetLOFrequency (theDevice, LMS_CH_RX,
	                                                 0, (float)frequency);
	if (res < 0) {
	   LMS_Close (theDevice);
	   throw FrequencyFailed();
	}

	res		= LMS_SetLPFBW (theDevice, LMS_CH_RX,
	                                               0, 1536000.0);
	if (res < 0) {
	   LMS_Close (theDevice);
	   throw BandwidthFailed();
	}

	LMS_SetGaindB (theDevice, LMS_CH_RX, 0, gain);

	LMS_Calibrate (theDevice, LMS_CH_RX, 0, 2500000.0, 0);

	theBuffer	= new RingBuffer<std::complex<float>> (64 * 32768);
	running. store (false);
}

	limeHandler::~limeHandler	(void) {
	running. store (false);
	usleep (500000);
	threadHandle. join ();
	LMS_Close (theDevice);
	delete theBuffer;
}


bool	limeHandler::restartReader	(int32_t freq) {
int	res;

	if (running. load ())
	   return true;

	res = LMS_SetLOFrequency (theDevice, LMS_CH_RX, 0, (float)freq);
	res = LMS_SetGaindB	   (theDevice, LMS_CH_RX, 0, gain);
	DEBUG_PRINT ("freq set to %d\n", freq);
	stream. isTx            = false;
        stream. channel         = 0;
        stream. fifoSize        = FIFO_SIZE;
        stream. throughputVsLatency     = 0.1;  // ???
        stream. dataFmt         = lms_stream_t::LMS_FMT_F32;    // floats
	res     = LMS_SetupStream (theDevice, &stream);
        if (res < 0)
           return false;
        res     = LMS_StartStream (&stream);
        if (res < 0)
           return false;

	threadHandle		= std::thread (&limeHandler::run, this);
	return true;
}

void	limeHandler::stopReader		(void) {
	if (!running. load ())
	   return;
	running. store (false);
	usleep (500000);
	threadHandle. join ();
}

int	limeHandler::getSamples		(std::complex<float> *v, int32_t a) {
	return theBuffer -> getDataFromBuffer (v, a);
}

int	limeHandler::Samples		(void) {
	return theBuffer -> GetRingBufferReadAvailable ();
}

void	limeHandler::resetBuffer	(void) {
	theBuffer	-> FlushRingBuffer ();
}

int16_t	limeHandler::bitDepth		(void) {
	return 12;
}

void	limeHandler::run	(void) {
int	res;
lms_stream_status_t streamStatus;
int	amountRead	= 0;

	DEBUG_PRINT ("lime is working now\n");
	running. store (true);
	while (running. load ()) {
	   res = LMS_RecvStream (&stream, localBuffer,
	                                     FIFO_SIZE,  &meta, 1000);
	   if (res > 0) {
	      theBuffer -> putDataIntoBuffer (localBuffer, res);
	      amountRead	+= res;
	      res	= LMS_GetStreamStatus (&stream, &streamStatus);
	   }
	}
	(void)LMS_StopStream	(&stream);
	(void)LMS_DestroyStream	(theDevice, &stream);
}
