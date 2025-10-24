#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *    Copyright (C) 2019 Amplifier, antenna and ppm correctors
 *    Fabio Capozzi
 *
 *    This file is part of dab-cmdline/dab-library
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

#include	"hackrf-handler.h"
#include  "device-exceptions.h"
#define	DEFAULT_GAIN	30

	hackrfHandler::hackrfHandler  (int32_t	frequency,
	                               int16_t	ppm,
	                               int16_t	lnaGain,
	                               int16_t	vgaGain,
	                               bool	ampEnable) {
int	res;
	vfoFrequency			= frequency;
	this	-> lnaGain		= lnaGain;
	this	-> vgaGain		= vgaGain;
	this	-> ampEnable		= ampEnable;

	this	-> inputRate		= 2048000;
	_I_Buffer	= new RingBuffer<std::complex<float>>(1024 * 1024);
//
	res	= hackrf_init ();
	if (res != HACKRF_SUCCESS) {
	   DEBUG_PRINT ("Problem with hackrf_init:");
	   DEBUG_PRINT ("%s \n", hackrf_error_name (hackrf_error (res)));
	   throw InitFailed();
	}

	res	= hackrf_open (&theDevice);
	if (res != HACKRF_SUCCESS) {
	   DEBUG_PRINT ("Problem with hackrf_open:");
	   DEBUG_PRINT ("%s \n",
	                 hackrf_error_name (hackrf_error (res)));
	   throw OpeningFailed();
	}

	res	= hackrf_set_sample_rate (theDevice, 2048000.0);
	if (res != HACKRF_SUCCESS) {
	   DEBUG_PRINT ("Problem with hackrf_set_samplerate:");
	   DEBUG_PRINT ("%s \n", hackrf_error_name (hackrf_error (res)));
	   throw SampleRateFailed();
	}

	res	= hackrf_set_baseband_filter_bandwidth (theDevice,
	                                                        1750000);
	if (res != HACKRF_SUCCESS) {
	   DEBUG_PRINT ("Problem with hackrf_set_bw:");
	   DEBUG_PRINT ("%s \n", hackrf_error_name (hackrf_error (res)));
	   throw BandwidthFailed();
	}

	res	= hackrf_set_freq (theDevice, frequency);
	if (res != HACKRF_SUCCESS) {
	   DEBUG_PRINT ("Problem with hackrf_set_freq: ");
	   DEBUG_PRINT ("%s \n", hackrf_error_name (hackrf_error (res)));
	   throw FrequencyFailed();
	}

	hackrf_device_list_t *deviceList = hackrf_device_list ();
	if (deviceList != nullptr) {	// well, it should be
	   char *serial = deviceList -> serial_numbers [0];
	   enum hackrf_usb_board_id board_id =
	                 deviceList -> usb_board_ids [0];
	   (void) serial;
	   (void) board_id;
	}

	if ((vgaGain <= 62) && (vgaGain >= 0))
           (void)hackrf_set_vga_gain (theDevice, vgaGain);
	if ((lnaGain <= 40) && (lnaGain >= 0))
           (void)hackrf_set_lna_gain (theDevice, lnaGain);
	(void)hackrf_set_amp_enable (theDevice, ampEnable);

	running. store (false);
}

	hackrfHandler::~hackrfHandler	(void) {
	stopReader ();
	if (_I_Buffer != nullptr)
	   delete _I_Buffer;
	hackrf_close (theDevice);
	hackrf_exit ();
}
//

void	hackrfHandler::setLNAGain	(int newGain) {
int	res;
	if ((newGain <= 40) && (newGain >= 0)) {
	   res	= hackrf_set_lna_gain (theDevice, newGain);
	   if (res != HACKRF_SUCCESS) {
	      DEBUG_PRINT ("Problem with hackrf_lna_gain :\n");
	      DEBUG_PRINT ("%s \n", hackrf_error_name (hackrf_error (res)));
	      return;
	   }
	}
}

void	hackrfHandler::setVGAGain	(int newGain) {
int	res;
	if ((newGain <= 62) && (newGain >= 0)) {
	   res	= hackrf_set_vga_gain (theDevice, newGain);
	   if (res != HACKRF_SUCCESS) {
	      DEBUG_PRINT ("Problem with hackrf_vga_gain :\n");
	      DEBUG_PRINT ("%s \n",
	                 hackrf_error_name (hackrf_error (res)));
	      return;
	   }
	}
}
//
//	we use a static large buffer, rather than trying to allocate
//	a buffer on the stack
static std::complex<float>buffer [32 * 32768];
static
int	callback (hackrf_transfer *transfer) {
hackrfHandler *ctx = static_cast <hackrfHandler *>(transfer -> rx_ctx);
int	i;
uint8_t *p	= transfer -> buffer;
RingBuffer<std::complex<float> > * q = ctx -> _I_Buffer;

	for (i = 0; i < transfer -> valid_length / 2; i ++) {
	   float re	= (((int8_t *)p) [2 * i]) / 128.0;
	   float im	= (((int8_t *)p) [2 * i + 1]) / 128.0;
	   buffer [i]	= std::complex<float> (re, im);
	}
	q -> putDataIntoBuffer (buffer, transfer -> valid_length / 2);
	return 0;
}

bool	hackrfHandler::restartReader	(int32_t newFrequency) {
int	res;

	if (running. load ())
	   return true;

	res     = hackrf_set_freq (theDevice, newFrequency);
        if (res != HACKRF_SUCCESS) {
           DEBUG_PRINT ("Problem with hackrf_set_freq: \n");
           DEBUG_PRINT ("%s \n", hackrf_error_name (hackrf_error (res)));
           return false;
        }
        vfoFrequency = newFrequency;

	res	= hackrf_start_rx (theDevice, callback, this);
	if (res != HACKRF_SUCCESS) {
	   DEBUG_PRINT ("Problem with hackrf_start_rx :\n");
	   DEBUG_PRINT ("%s \n", hackrf_error_name (hackrf_error (res)));
	   return false;
	}
//
//	reset the gain(s), since the amp-enable is to be reset
//	after each change in frequency
	if ((vgaGain <= 62) && (vgaGain >= 0))
           (void)hackrf_set_vga_gain (theDevice, vgaGain);
	if ((lnaGain <= 40) && (lnaGain >= 0))
           (void)hackrf_set_lna_gain (theDevice, lnaGain);
	(void)hackrf_set_amp_enable (theDevice, ampEnable);
	running. store (hackrf_is_streaming (theDevice));
	return running. load ();
}

void	hackrfHandler::stopReader	(void) {
int	res;

	if (!running. load ())
	   return;

	res	= hackrf_stop_rx (theDevice);
	if (res != HACKRF_SUCCESS) {
	   DEBUG_PRINT ("Problem with hackrf_stop_rx :");
	   DEBUG_PRINT ("%s \n", hackrf_error_name (hackrf_error (res)));
	   return;
	}
	running. store (false);
}

//
//	The brave old getSamples. For the hackrf, we get
//	size still in I/Q pairs
int32_t	hackrfHandler::getSamples (std::complex<float> *V, int32_t size) {
	return _I_Buffer	-> getDataFromBuffer (V, size);
}

int32_t	hackrfHandler::Samples	(void) {
	return _I_Buffer	-> GetRingBufferReadAvailable ();
}

void	hackrfHandler::resetBuffer	(void) {
	_I_Buffer	-> FlushRingBuffer ();
}

int16_t	hackrfHandler::bitDepth	(void) {
	return 8;
}
