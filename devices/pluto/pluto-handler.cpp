#
/*
 *    Copyright (C) 2014 .. 2020
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of dab-cmdline
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

#include	"pluto-handler.h"
#include	<unistd.h>

/* static scratch mem for strings */
static char tmpstr[64];

/* helper function generating channel names */
static
char*	get_ch_name (const char* type, int id) {
        snprintf (tmpstr, sizeof(tmpstr), "%s%d", type, id);
        return tmpstr;
}

/* returns ad9361 phy device */
static
struct	iio_device* get_ad9361_phy (struct iio_context *ctx) {
struct iio_device *dev =  iio_context_find_device(ctx, "ad9361-phy");
	if (dev == nullptr) {
	   fprintf (stderr, "No ad9361-phy found, fatal\n");
	   throw (21);
	}
        return dev;
}

/* applies streaming configuration through IIO */
static
bool	cfg_ad9361_streaming_ch (struct iio_context *ctx,
	                         struct stream_cfg *cfg, int chid) {
struct iio_channel *chn = nullptr;

// Configure phy and lo channels
//	fprintf (stderr, "* Acquiring AD9361 phy channel %d\n", chid);
	chn = iio_device_find_channel (get_ad9361_phy (ctx),
                                        get_ch_name ("voltage", chid),
                                        false);
	if (chn == nullptr) {
	   fprintf (stderr, "cannot acquire phy channel %d\n", chid);
	   return false;
	}

	if (iio_channel_attr_write (chn, "rf_port_select",
	                                                cfg -> rfport) < 0) {
	   fprintf (stderr, "cannot select port\n");
	   return false;
	}

	if (iio_channel_attr_write_longlong (chn, "rf_bandwidth",
	                                                 cfg -> bw_hz) < 0) {
	   fprintf (stderr, "cannot select bandwidth\n");
	   return false;
	}

	if (iio_channel_attr_write_longlong (chn, "sampling_frequency",
	                                                 cfg -> fs_hz) < 0) {
	   fprintf (stderr, "cannot set sampling frequency\n");
	   return false;
	}

	cfg	-> gain_channel = chn;
// Configure LO channel
//	fprintf (stderr, "* Acquiring AD9361 %s lo channel\n", "RX");
	cfg -> lo_channel = iio_device_find_channel (get_ad9361_phy (ctx),
                                              get_ch_name ("altvoltage", 0),
                                              true);
	if (cfg -> lo_channel == nullptr) {
	   fprintf (stderr, "cannot find lo for channel\n");
	   return false;
	}

	if (iio_channel_attr_write_longlong (cfg -> lo_channel, "frequency",
	                                                 cfg -> lo_hz) < 0) {
	   fprintf (stderr, "cannot set local oscillator frequency\n");
	   return false;
	}
	return true;
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

	plutoHandler::plutoHandler  (int32_t	frequency,
	                             int	gainValue,
	                             bool	agcMode):
	                                  _I_Buffer (4 * 1024 * 1024) {
	ctx				= nullptr;
	rxbuf				= nullptr;
	rx0_i				= nullptr;
	rx0_q				= nullptr;

	rxcfg. bw_hz			= PLUTO_RATE;
	rxcfg. fs_hz			= PLUTO_RATE;
	rxcfg. lo_hz			= frequency;
	rxcfg. rfport			= "A_BALANCED";

	ctx	= iio_create_default_context ();
	if (ctx == nullptr) {
	   ctx = iio_create_local_context ();
	}

	if (ctx == nullptr) {
	   ctx = iio_create_network_context ("pluto.local");
	}

	if (ctx == nullptr) {
//	   ctx = iio_create_network_context ("192.168.2.1");
	   ctx = iio_create_network_context ("qra.f5oeo.fr");
	}

	if (ctx == nullptr) {
	   fprintf (stderr, "No pluto found, fatal\n");
	   throw (24);
	}

	if (iio_context_get_devices_count (ctx) <= 0) {
	   fprintf (stderr, "no devices found, fatal");
	   throw (25);
	}

	rx = iio_context_find_device (ctx, "cf-ad9361-lpc");
	if (rx == nullptr) {
	   fprintf (stderr, "No device found");
	   throw (26);
	}

	fprintf (stderr, "found a device %s\n",
	               iio_device_get_name (rx));
        if (!cfg_ad9361_streaming_ch (ctx, &rxcfg, 0)) {
	   fprintf (stderr, "RX port 0 not found");
	   throw (27);
	}

        if (!get_ad9361_stream_ch (ctx, rx, 0, &rx0_i)) {
	   fprintf (stderr, "RX chan i not found");
	   throw (28);
	}

        if (!get_ad9361_stream_ch (ctx, rx, 1, &rx0_q)) {
	   fprintf (stderr, "RX chan q not found");
	   throw (29);
	}

        iio_channel_enable (rx0_i);
        iio_channel_enable (rx0_q);

        rxbuf = iio_device_create_buffer (rx, 1024*1024, false);
	if (rxbuf == nullptr) {
	   fprintf (stderr, "could not create RX buffer, fatal");
	   throw (30);
	}
//
	iio_buffer_set_blocking_mode (rxbuf, true);
	if (!agcMode) {
	   int ret = iio_channel_attr_write (rxcfg. gain_channel,
	                                     "gain_control_mode", "manual");
	   ret = iio_channel_attr_write_longlong (rxcfg. gain_channel,
	                                          "hardwaregain", gainValue);
	   if (ret < 0) 
	      fprintf (stderr, "error in initial gain setting");
	}
	else {
	   int ret = iio_channel_attr_write (rxcfg. gain_channel,
	                                     "gain_control_mode", "slow_attack");
	   if (ret < 0)
	      fprintf (stderr, "error in initial gain setting");
	}
	
	float	divider		= (float)DIVIDER;
	float	denominator	= DAB_RATE / divider;
	for (int i = 0; i < DAB_RATE / DIVIDER; i ++) {
           float inVal  = float (PLUTO_RATE / divider);
           mapTable_int [i]     =  int (floor (i * (inVal / denominator)));
           mapTable_float [i]   = i * (inVal / denominator) - mapTable_int [i];
        }
        convIndex       = 0;
	running. store (false);
}

	plutoHandler::~plutoHandler () {
	stopReader ();
	iio_buffer_destroy (rxbuf);
	iio_context_destroy (ctx);
}
//

bool	plutoHandler::restartReader	(int32_t freq) {
	if (running. load())
	   return true;		// should not happen

	if (freq == rxcfg. lo_hz)
	   return true;

	rxcfg. lo_hz	= freq;
	int ret	= iio_channel_attr_write_longlong
	                             (rxcfg. lo_channel,
	                                   "frequency", rxcfg. lo_hz);
	if (ret < 0) {
	   fprintf (stderr, "error in selected frequency\n");
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

int32_t	plutoHandler::getSamples (std::complex<float> *V, int32_t size) { 
	if (!running. load ())
	   return 0;
	return _I_Buffer. getDataFromBuffer (V, size);
}

int32_t	plutoHandler::Samples () {
	return _I_Buffer. GetRingBufferReadAvailable();
}

void	plutoHandler::resetBuffer() {
	_I_Buffer. FlushRingBuffer();
}

int16_t	plutoHandler::bitDepth() {
	return 12;
}

