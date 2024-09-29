#
/*
 *    Copyright (C) 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the DAB library
 *
 *    DAB library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation version 2 of the License.
 *
 *    DAB library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB library if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"pluto-handler.h"
#include	<unistd.h>
#include	"dabFilter.h"
#include  "device-exceptions.h"

/* static scratch mem for strings */
static char tmpstr[64];

/* helper function generating channel names */
static
char*	get_ch_name (const char* type, int id) {
        snprintf (tmpstr, sizeof(tmpstr), "%s%d", type, id);
        return tmpstr;
}

/* finds AD9361 streaming IIO channels */
static
bool get_ad9361_stream_ch (__notused struct iio_context *ctx,
	                   struct iio_device *dev,
	                   int chid, struct iio_channel **chn) {
        *chn = iio_device_find_channel (dev,
	                                get_ch_name ("voltage", chid), false);
        if (*chn == nullptr)
	   *chn = iio_device_find_channel (dev,
	                                   get_ch_name ("altvoltage", chid),
	                                   false);
        return *chn != nullptr;
}

static inline
std::complex<float> cmul (std::complex<float> x, float y) {
	return std::complex<float> (real (x) * y, imag (x) * y);
}

int	ad9361_set_trx_fir_enable(struct iio_device *dev, int enable) {
int ret = iio_device_attr_write_bool (dev,
	                              "in_out_voltage_filter_fir_en",
	                              !!enable);
	if (ret < 0)
	   ret = iio_channel_attr_write_bool (
	                        iio_device_find_channel(dev, "out", false),
	                        "voltage_filter_fir_en", !!enable);
	return ret;
}

int	ad9361_get_trx_fir_enable (struct iio_device *dev, int *enable) {
bool value;

	int ret = iio_device_attr_read_bool (dev,
	                                     "in_out_voltage_filter_fir_en",
	                                     &value);

	if (ret < 0)
	   ret = iio_channel_attr_read_bool (
	                        iio_device_find_channel (dev, "out", false),
	                        "voltage_filter_fir_en", &value);
	if (!ret)
	   *enable	= value;

	return ret;
}


	plutoHandler::plutoHandler  (int32_t	frequency,
	                             int	gainValue,
	                             bool	agcMode):
	                               _I_Buffer (4 * 1024 * 1024) {
struct iio_channel *chn = nullptr;

	this	-> ctx			= nullptr;
	this	-> rxbuf		= nullptr;
	this	-> rx0_i		= nullptr;
	this	-> rx0_q		= nullptr;

	this	-> bw_hz		= PLUTO_RATE;
	this	-> fs_hz		= PLUTO_RATE;
	this	-> lo_hz		= frequency;
	this	-> rfport		= "A_BALANCED";

//
//	step 1: establish a context
//
	ctx	= iio_create_default_context ();
	if (ctx == nullptr) {
	   ctx = iio_create_local_context ();
	}

	if (ctx == nullptr) {
	   ctx = iio_create_network_context ("pluto.local");
	}

	if (ctx == nullptr) {
	   ctx = iio_create_network_context ("192.168.2.1");
//	   ctx = iio_create_network_context ("qra.f5oeo.fr");
	}

	if (ctx == nullptr) {
	   DEBUG_PRINT ("No pluto found, fatal\n");
	   throw NoInstanceFound();
	}
//
//	step 2: look for availability of devices
	if (iio_context_get_devices_count (ctx) <= 0) {
	   DEBUG_PRINT ("no devices found, fatal");
	   throw DeviceNotFound();
	}

	rx = iio_context_find_device (ctx, "cf-ad9361-lpc");
	if (rx == nullptr) {
	   DEBUG_PRINT ("No device found");
	   iio_context_destroy (ctx);
	   throw DeviceNotFound();
	}

	DEBUG_PRINT ("found a device %s\n", iio_device_get_name (rx));

//	DEBUG_PRINT ("* Acquiring AD9361 phy channel %d\n", 0);
	struct iio_device *phys_dev = iio_context_find_device (ctx,
	                                             "ad9361-phy");
	if (phys_dev == nullptr) {
	   DEBUG_PRINT ("cannot find device\n");
	   iio_context_destroy (ctx);
	   throw DeviceNotFound();
	}
	chn = iio_device_find_channel (phys_dev,
                                       get_ch_name ("voltage", 0),
                                       false);
	if (chn == nullptr) {
	   DEBUG_PRINT ("cannot acquire phy channel %d\n", 0);
	   iio_context_destroy (ctx);
	   throw OpeningFailed();
	}

	if (iio_channel_attr_write (chn, "rf_port_select",
	                                            this -> rfport) < 0) {
	   DEBUG_PRINT ("cannot select port\n");
	   iio_context_destroy (ctx);
	   throw PortSelectFailed();
	}

	if (iio_channel_attr_write_longlong (chn, "rf_bandwidth",
	                                             this -> bw_hz) < 0) {
	   DEBUG_PRINT ("cannot select bandwidth\n");
	   iio_context_destroy (ctx);
	   throw BandwidthFailed();
	}

	if (iio_channel_attr_write_longlong (chn, "sampling_frequency",
	                                                 this -> fs_hz) < 0) {
	   DEBUG_PRINT ("cannot set sampling frequency\n");
	   iio_context_destroy (ctx);
	   throw SampleRateFailed();
	}
//
//	we use the channel for setting gains
	this	-> gain_channel = chn;
// Configure LO channel, when here, we know that phys_dev is not null
//	DEBUG_PRINT ("* Acquiring AD9361 %s lo channel\n", "RX");
	this -> lo_channel =
	                iio_device_find_channel (phys_dev,
                                                 get_ch_name ("altvoltage", 0),
                                                 true);
	if (this -> lo_channel == nullptr) {
	   DEBUG_PRINT ("cannot find lo for channel\n");
	   iio_context_destroy (ctx);
	   throw InterfaceFailed();
	}

	if (iio_channel_attr_write_longlong (this -> lo_channel,
	                                     "frequency", this -> lo_hz) < 0) {
	   DEBUG_PRINT ("cannot set local oscillator frequency\n");
	   iio_context_destroy (ctx);
	   throw FrequencyFailed();
	}

        if (!get_ad9361_stream_ch (ctx, rx, 0, &rx0_i)) {
	   DEBUG_PRINT ("RX chan i not found\n");
	   iio_context_destroy (ctx);
	   throw OpeningChannelFailed("RX","i");
	}

        if (!get_ad9361_stream_ch (ctx, rx, 1, &rx0_q)) {
	   DEBUG_PRINT ("RX chan q not found");
	   iio_context_destroy (ctx);
	   throw OpeningChannelFailed("RX","q");
	}

        iio_channel_enable (rx0_i);
        iio_channel_enable (rx0_q);

        rxbuf = iio_device_create_buffer (rx, 256*1024, false);
	if (rxbuf == nullptr) {
	   DEBUG_PRINT ("could not create RX buffer, fatal");
	   iio_context_destroy (ctx);
	   throw CreatingBufferFailed();
	}
//
	iio_buffer_set_blocking_mode (rxbuf, true);
	if (!agcMode) {
	   int ret = iio_channel_attr_write (this -> gain_channel,
	                                     "gain_control_mode", "manual");
	   ret = iio_channel_attr_write_longlong (this -> gain_channel,
	                                          "hardwaregain", gainValue);
	   if (ret < 0)
	      DEBUG_PRINT ("error in initial gain setting");
	}
	else {
	   int ret = iio_channel_attr_write (this -> gain_channel,
	                                     "gain_control_mode",
	                                     "slow_attack");
	   if (ret < 0)
	      DEBUG_PRINT ("error in initial gain setting");
	}
//
//	and set up interpolation table
	float	divider		= (float)DIVIDER;
	float	denominator	= DAB_RATE / divider;
	for (int i = 0; i < DAB_RATE / DIVIDER; i ++) {
           float inVal  = float (PLUTO_RATE / divider);
           mapTable_int [i]     =  int (floor (i * (inVal / denominator)));
           mapTable_float [i]   = i * (inVal / denominator) - mapTable_int [i];
        }
        convIndex       = 0;

	int enabled;
//	go for the filter
	ad9361_get_trx_fir_enable (phys_dev, &enabled);
	if (enabled)
	   ad9361_set_trx_fir_enable (phys_dev, 0);
	int ret = iio_device_attr_write_raw (phys_dev,
	                                     "filter_fir_config",
	                                     dabFilter, strlen (dabFilter));
	if (ret < 0)
	   DEBUG_PRINT ("filter mislukt");
//	and enable it
	ad9361_set_trx_fir_enable (phys_dev, 1);
	running. store (false);
}

	plutoHandler::~plutoHandler () {
	stopReader ();
	iio_buffer_destroy (rxbuf);
	iio_context_destroy (ctx);
}

bool	plutoHandler::restartReader	(int32_t freq) {
	if (running. load())
	   return true;		// should not happen

//	if (freq == this -> lo_hz)
//	   return true;

	this -> lo_hz	= freq;
	int ret	= iio_channel_attr_write_longlong
	                             (this -> lo_channel,
	                                   "frequency", this -> lo_hz);
	if (ret < 0) {
	   DEBUG_PRINT ("error in selected frequency\n");
	   return false;
	}

	threadHandle	= std::thread (&plutoHandler::run, this);
	return true;
}

void	plutoHandler::stopReader() {
	if (!running. load())
	   return;
	running. store (false);
	usleep (50000);
	threadHandle. join ();
}

int32_t	plutoHandler::defaultFrequency	() {
	return 220000000;
}

void	plutoHandler::run	() {
char	*p_end, *p_dat;
int	p_inc;
int	nbytes_rx;
std::complex<float> localBuf [DAB_RATE / DIVIDER];

	running. store (true);
	while (running. load ()) {
	   nbytes_rx	= iio_buffer_refill	(rxbuf);
	   p_inc	= iio_buffer_step	(rxbuf);
	   p_end	= (char *)(iio_buffer_end  (rxbuf));

	   for (p_dat = (char *)iio_buffer_first (rxbuf, rx0_i);
	        p_dat < p_end; p_dat += p_inc) {
	      const int16_t i_p = ((int16_t *)p_dat) [0];
	      const int16_t q_p = ((int16_t *)p_dat) [1];
	      std::complex<float>sample = std::complex<float> (i_p / 2048.0,
	                                                       q_p / 2048.0);
	      convBuffer [convIndex ++] = sample;
	      if (convIndex > CONV_SIZE) {
	         for (int j = 0; j < DAB_RATE / DIVIDER; j ++) {
	            int16_t inpBase	= mapTable_int [j];
	            float   inpRatio	= mapTable_float [j];
	            localBuf [j]	= cmul (convBuffer [inpBase + 1],
	                                                          inpRatio) +
                                     cmul (convBuffer [inpBase], 1 - inpRatio);
                 }
	         _I_Buffer. putDataIntoBuffer (localBuf,
	                                          DAB_RATE / DIVIDER);
	         convBuffer [0] = convBuffer [CONV_SIZE];
	         convIndex = 1;
	      }
	   }
	}
}

int32_t	plutoHandler::Samples		() {
	return _I_Buffer. GetRingBufferReadAvailable ();
}

int32_t	plutoHandler::getSamples	(std::complex<float> *V, int size) {
	return _I_Buffer. getDataFromBuffer (V, size);
}

void	plutoHandler::resetBuffer	() {
	_I_Buffer. FlushRingBuffer ();
}

int16_t	plutoHandler::bitDepth		() { return 12;}
