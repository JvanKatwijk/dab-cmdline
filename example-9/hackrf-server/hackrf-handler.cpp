#
/*
 *    Copyright (C) 2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of hackrf transmitter
 *
 *    hackrf transmitter is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation version 2 of the License.
 *
 *    hackrf transmitter is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with hackrf transmitter if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"hackrf-handler.h"
#include	<unistd.h>

#define	DEFAULT_GAIN	30

	hackrfHandler::hackrfHandler  (int frequency,
	                               int rate, int gain) {
int	err;
int	res;
	this	-> inputRate		= rate;

	_I_Buffer	= new RingBuffer<std::complex<float>>(1024 * 1024);
	vfoFrequency	= frequency;
#ifdef  __MINGW32__
        const char *libraryString = "libhackrf.dll";
        Handle          = LoadLibrary ((wchar_t *)L"libhackrf.dll");
#elif  __clang__
        const char *libraryString = "/opt/local/lib/libhackrf.dylib";
        Handle = dlopen(libraryString,RTLD_NOW);
#else
        const char *libraryString = "libhackrf.so";
        Handle          = dlopen (libraryString, RTLD_NOW);
#endif

        if (Handle == nullptr) {
           fprintf (stderr, "failed to open %s\n", libraryString);
           throw (20);
        }

       libraryLoaded   = true;
        if (!load_hackrfFunctions ()) {
#ifdef __MINGW32__
           FreeLibrary (Handle);
#else
           dlclose (Handle);
#endif
           throw (21);
        }
//
//      From here we have a library available
	res	= hackrf_init ();
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_init:");
	   fprintf (stderr, "%s \n",
	                 hackrf_error_name (hackrf_error (res)));
	   throw (21);
	}

	res	= hackrf_open (&theDevice);
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_open:");
	   fprintf (stderr, "%s \n",
	                 this -> hackrf_error_name (hackrf_error (res)));
	   throw (22);
	}

	res	= hackrf_set_sample_rate (theDevice, (float)inputRate);
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_set_samplerate:");
	   fprintf (stderr, "%s \n",
	                 hackrf_error_name (hackrf_error (res)));
	   throw (23);
	}
	fprintf (stderr, "samplerate set to %d\n", inputRate);

	res	= hackrf_set_baseband_filter_bandwidth (theDevice,
	                                                        1750000);
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_set_bw:");
	   fprintf (stderr, "%s \n",
	                 hackrf_error_name (hackrf_error (res)));
	   throw (24);
	}

	res	= hackrf_set_freq (theDevice, vfoFrequency);
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_set_freq: ");
	   fprintf (stderr, "%s \n",
	                 this -> hackrf_error_name (hackrf_error (res)));
	   throw (25);
	}

	hackrf_device_list_t *deviceList = hackrf_device_list ();
	if (deviceList != nullptr) {	// well, it should be
	   char *serial = deviceList -> serial_numbers [0];
//	   serial_number_display -> setText (serial);
//	   enum hackrf_usb_board_id board_id =
//	                 deviceList -> usb_board_ids [0];
//	   usb_board_id_display ->
//	                setText (this -> hackrf_usb_board_id_name (board_id));
	}

	running. store (false);
}

	hackrfHandler::~hackrfHandler	(void) {
	stop ();
	if (_I_Buffer != nullptr)
	   delete _I_Buffer;
	hackrf_close (theDevice);
	hackrf_exit ();
}
//

//	we use a static large buffer, rather than trying to allocate
//	a buffer on the stack
static std::complex<float>buffer [32 * 32768];
static
int	callback (hackrf_transfer *transfer) {
hackrfHandler *ctx = static_cast <hackrfHandler *>(transfer -> tx_ctx);
int	i;
uint8_t *p	= transfer -> buffer;
RingBuffer<std::complex<float>> * q = ctx -> _I_Buffer;
int32_t amount;

	amount = q -> getDataFromBuffer (buffer, transfer -> valid_length / 2);
	for (i = 0; i < amount; i ++) {
	   p [2 * i]     = (uint8_t)((int8_t)(real (buffer [i]) * 128.0));
	   p [2 * i + 1] = (uint8_t)((int8_t)(imag (buffer [i]) * 128.0));
	}
	for (i = amount; i < transfer -> valid_length / 2; i ++) {
	   p [2 * i]	= 0;
	   p [2 * i + 1] = 0;
	}
	return 0;
}

bool	hackrfHandler::start (uint64_t frequency, int gain) {
int	res;

	if (running. load ())
	   return true;

	(void) hackrf_set_freq (theDevice, frequency);
	if ((gain > 0 ) && (gain <= 47)) 
	   hackrf_set_txvga_gain (theDevice, gain);

	res  = hackrf_start_tx (theDevice, callback, this);	
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_start_tx :\n");
	   fprintf (stderr, "%s \n",
	                 this -> hackrf_error_name (hackrf_error (res)));
	   return false;
	}

	fprintf (stderr, "transmitter running\n");
	running. store (hackrf_is_streaming (theDevice));
	fprintf (stderr, "hackrf is %s transmitting\n",
	           running.load () ? " " : "not");
	return running. load ();
}

void	hackrfHandler::stop	(void) {
int	res;

	if (!running. load ())
	   return;

	res	= hackrf_stop_tx (theDevice);
	if (res != HACKRF_SUCCESS) {
	   fprintf (stderr, "Problem with hackrf_stop_tx :\n", res);
	   fprintf (stderr, "%s \n",
	                 hackrf_error_name (hackrf_error (res)));
	   return;
	}
	running. store (false);
}

void	hackrfHandler::passOn	(std::complex<float> *insamples, int amount) {
	while (_I_Buffer -> GetRingBufferWriteAvailable () < amount)
	   usleep (1000);
	_I_Buffer	-> putDataIntoBuffer (insamples, amount);
}

bool	hackrfHandler::load_hackrfFunctions (void) {
//
//	link the required procedures
	this -> hackrf_init	= (pfn_hackrf_init)
	                       GETPROCADDRESS (Handle, "hackrf_init");
	if (this -> hackrf_init == nullptr) {
	   fprintf (stderr, "Could not find hackrf_init\n");
	   return false;
	}

	this -> hackrf_open	= (pfn_hackrf_open)
	                       GETPROCADDRESS (Handle, "hackrf_open");
	if (this -> hackrf_open == nullptr) {
	   fprintf (stderr, "Could not find hackrf_open\n");
	   return false;
	}

	this -> hackrf_close	= (pfn_hackrf_close)
	                       GETPROCADDRESS (Handle, "hackrf_close");
	if (this -> hackrf_close == nullptr) {
	   fprintf (stderr, "Could not find hackrf_close\n");
	   return false;
	}

	this -> hackrf_exit	= (pfn_hackrf_exit)
	                       GETPROCADDRESS (Handle, "hackrf_exit");
	if (this -> hackrf_exit == nullptr) {
	   fprintf (stderr, "Could not find hackrf_exit\n");
	   return false;
	}

	this -> hackrf_start_rx	= (pfn_hackrf_start_rx)
	                       GETPROCADDRESS (Handle, "hackrf_start_rx");
	if (this -> hackrf_start_rx == nullptr) {
	   fprintf (stderr, "Could not find hackrf_start_rx\n");
	   return false;
	}

	this -> hackrf_start_tx	= (pfn_hackrf_start_tx)
	                       GETPROCADDRESS (Handle, "hackrf_start_tx");
	if (this -> hackrf_start_tx == nullptr) {
	   fprintf (stderr, "Could not find hackrf_start_tx\n");
	   return false;
	}

	this -> hackrf_stop_rx	= (pfn_hackrf_stop_rx)
	                       GETPROCADDRESS (Handle, "hackrf_stop_rx");
	if (this -> hackrf_stop_rx == nullptr) {
	   fprintf (stderr, "Could not find hackrf_stop_rx\n");
	   return false;
	}

	this -> hackrf_stop_tx	= (pfn_hackrf_stop_tx)
	                           GETPROCADDRESS (Handle, "hackrf_stop_tx");
	if (this -> hackrf_stop_tx == nullptr) {
	   fprintf (stderr, "Could not find hackrf_stop_tx\n");
	   return false;
	}

	this -> hackrf_device_list	= (pfn_hackrf_device_list)
	                       GETPROCADDRESS (Handle, "hackrf_device_list");
	if (this -> hackrf_device_list == nullptr) {
	   fprintf (stderr, "Could not find hackrf_device_list\n");
	   return false;
	}

	this -> hackrf_set_baseband_filter_bandwidth	=
	                      (pfn_hackrf_set_baseband_filter_bandwidth)
	                      GETPROCADDRESS (Handle,
	                         "hackrf_set_baseband_filter_bandwidth");
	if (this -> hackrf_set_baseband_filter_bandwidth == nullptr) {
	   fprintf (stderr, "Could not find hackrf_set_baseband_filter_bandwidth\n");
	   return false;
	}

	this -> hackrf_set_txvga_gain	= (pfn_hackrf_set_txvga_gain)
	                       GETPROCADDRESS (Handle, "hackrf_set_vga_gain");
	if (this -> hackrf_set_txvga_gain == nullptr) {
	   fprintf (stderr, "Could not find hackrf_set_txvga_gain\n");
	   return false;
	}

	this -> hackrf_set_freq	= (pfn_hackrf_set_freq)
	                       GETPROCADDRESS (Handle, "hackrf_set_freq");
	if (this -> hackrf_set_freq == nullptr) {
	   fprintf (stderr, "Could not find hackrf_set_freq\n");
	   return false;
	}

	this -> hackrf_set_sample_rate	= (pfn_hackrf_set_sample_rate)
	                       GETPROCADDRESS (Handle, "hackrf_set_sample_rate");
	if (this -> hackrf_set_sample_rate == nullptr) {
	   fprintf (stderr, "Could not find hackrf_set_sample_rate\n");
	   return false;
	}

	this -> hackrf_is_streaming	= (pfn_hackrf_is_streaming)
	                       GETPROCADDRESS (Handle, "hackrf_is_streaming");
	if (this -> hackrf_is_streaming == nullptr) {
	   fprintf (stderr, "Could not find hackrf_is_streaming\n");
	   return false;
	}

	this -> hackrf_error_name	= (pfn_hackrf_error_name)
	                       GETPROCADDRESS (Handle, "hackrf_error_name");
	if (this -> hackrf_error_name == nullptr) {
	   fprintf (stderr, "Could not find hackrf_error_name\n");
	   return false;
	}

	this -> hackrf_usb_board_id_name = (pfn_hackrf_usb_board_id_name)
	                       GETPROCADDRESS (Handle, "hackrf_usb_board_id_name");
	if (this -> hackrf_usb_board_id_name == nullptr) {
	   fprintf (stderr, "Could not find hackrf_usb_board_id_name\n");
	   return false;
	}
// Aggiunta Fabio
//	this -> hackrf_set_antenna_enable = (pfn_hackrf_set_antenna_enable)
//	                  GETPROCADDRESS (Handle, "hackrf_set_antenna_enable");
//	if (this -> hackrf_set_antenna_enable == nullptr) {
//	   fprintf (stderr, "Could not find hackrf_set_antenna_enable\n");
//	   return false;
//	}
//
//	this -> hackrf_set_amp_enable = (pfn_hackrf_set_amp_enable)
//	                  GETPROCADDRESS (Handle, "hackrf_set_amp_enable");
//	if (this -> hackrf_set_amp_enable == nullptr) {
//	   fprintf (stderr, "Could not find hackrf_set_amp_enable\n");
//	   return false;
//	}
//
//	this -> hackrf_si5351c_read = (pfn_hackrf_si5351c_read)
//	                 GETPROCADDRESS (Handle, "hackrf_si5351c_read");
//	if (this -> hackrf_si5351c_read == nullptr) {
//	   fprintf (stderr, "Could not find hackrf_si5351c_read\n");
//	   return false;
//	}
//
//	this -> hackrf_si5351c_write = (pfn_hackrf_si5351c_write)
//	                 GETPROCADDRESS (Handle, "hackrf_si5351c_write");
//	if (this -> hackrf_si5351c_write == nullptr) {
//	   fprintf (stderr, "Could not find hackrf_si5351c_write\n");
//	   return false;
//	}
//
	fprintf (stderr, "OK, functions seem to be loaded\n");
	return true;
}
