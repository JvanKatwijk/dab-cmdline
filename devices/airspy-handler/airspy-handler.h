
/**
 *  IW0HDV Extio
 *
 *  Copyright 2015 by Andrea Montefusco IW0HDV
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 *
 *	recoding and taking parts for the airspyRadio interface
 *	for the DAB library
 *	jan van Katwijk
 *	Lazy Chair Computing
 */
#pragma once

#include	"ringbuffer.h"
#include	"device-handler.h"
#include	<complex>

#ifdef  __MINGW32__
#include        "windows.h"
#else
#ifndef __FREEBSD__
#include        "alloca.h"
#endif
#include        "dlfcn.h"
typedef void    *HINSTANCE;
#endif

#ifndef	__MINGW32__
#include	"libairspy/airspy.h"
#else
#include	"airspy.h"
#endif

extern "C"  {
typedef	int (*pfn_airspy_init) (void);
typedef int (*pfn_airspy_exit) (void);
typedef int (*pfn_airspy_open) (struct airspy_device**);
typedef int (*pfn_airspy_close) (struct airspy_device*);
typedef int (*pfn_airspy_get_samplerates) (struct airspy_device* device,
	                                   uint32_t* buffer,
	                                   const uint32_t len);
typedef int (*pfn_airspy_set_samplerate) (struct airspy_device* device,
	                                  uint32_t samplerate);
typedef int (*pfn_airspy_start_rx) (struct airspy_device* device,
	                            airspy_sample_block_cb_fn callback,
	                            void* rx_ctx);
typedef bool (*pfn_airspy_is_streaming)(struct airspy_device *device);
typedef int (*pfn_airspy_stop_rx) (struct airspy_device* device);

typedef int (*pfn_airspy_set_sample_type) (struct airspy_device *,
	                                   airspy_sample_type);
typedef int (*pfn_airspy_set_freq) (struct airspy_device* device,
	                            const uint32_t freq_hz);

typedef int (*pfn_airspy_set_lna_gain) (struct airspy_device* device,
	                                uint8_t value);

typedef int (*pfn_airspy_set_mixer_gain) (struct airspy_device* device,
	                                  uint8_t value);

typedef int (*pfn_airspy_set_vga_gain) (struct airspy_device*
	                                device, uint8_t
	                                value);
typedef int (*pfn_airspy_set_lna_agc) (struct airspy_device* device,
	                               uint8_t value);
typedef int (*pfn_airspy_set_mixer_agc) (struct airspy_device* device,
	                                 uint8_t value);

typedef int (*pfn_airspy_set_rf_bias) (struct airspy_device* dev,
	                               uint8_t value);

typedef const char* (*pfn_airspy_error_name) (enum airspy_error errcode);
typedef int (*pfn_airspy_board_id_read) (struct airspy_device *,
	                                         uint8_t *);
typedef const char* (*pfn_airspy_board_id_name) (enum airspy_board_id board_id);
typedef int (*pfn_airspy_board_partid_serialno_read)(struct airspy_device* device, airspy_read_partid_serialno_t* read_partid_serialno);

typedef int (*pfn_airspy_set_linearity_gain) (struct airspy_device* device, uint8_t value);
typedef int (*pfn_airspy_set_sensitivity_gain)(struct airspy_device* device, uint8_t value);
}

class airspyHandler: public deviceHandler {
public:
			airspyHandler		(int32_t, int16_t,
	                                          int16_t, bool);
			~airspyHandler		(void);
	bool		restartReader		(int32_t);
	void		stopReader		(void);
	int32_t		getSamples		(std::complex<float> *v,
	                                                     int32_t size);
	int32_t		Samples			(void);
	void		resetBuffer		(void);
	int16_t		bitDepth		(void);
	void		setGain			(int32_t);
private:
	bool		load_airspyFunctions	(void);
//	The functions to be extracted from the dll/.so file
	pfn_airspy_init		   my_airspy_init;
	pfn_airspy_exit		   my_airspy_exit;
	pfn_airspy_error_name	   my_airspy_error_name;
	pfn_airspy_open		   my_airspy_open;
	pfn_airspy_close	   my_airspy_close;
	pfn_airspy_get_samplerates my_airspy_get_samplerates;
	pfn_airspy_set_samplerate  my_airspy_set_samplerate;
	pfn_airspy_start_rx	   my_airspy_start_rx;
	pfn_airspy_is_streaming	   my_airspy_is_streaming;
	pfn_airspy_stop_rx	   my_airspy_stop_rx;
	pfn_airspy_set_sample_type my_airspy_set_sample_type;
	pfn_airspy_set_freq	   my_airspy_set_freq;
	pfn_airspy_set_lna_gain	   my_airspy_set_lna_gain;
	pfn_airspy_set_mixer_gain  my_airspy_set_mixer_gain;
	pfn_airspy_set_vga_gain	   my_airspy_set_vga_gain;
	pfn_airspy_set_linearity_gain my_airspy_set_linearity_gain;
	pfn_airspy_set_sensitivity_gain my_airspy_set_sensitivity_gain;
	pfn_airspy_set_lna_agc	   my_airspy_set_lna_agc;
	pfn_airspy_set_mixer_agc   my_airspy_set_mixer_agc;
	pfn_airspy_set_rf_bias	   my_airspy_set_rf_bias;
	pfn_airspy_board_id_read   my_airspy_board_id_read;
	pfn_airspy_board_id_name   my_airspy_board_id_name;
	pfn_airspy_board_partid_serialno_read
		                   my_airspy_board_partid_serialno_read;
//
	int32_t		frequency;
	int16_t		ppmCorrection;
	int16_t		gain;
	HINSTANCE	Handle;
	bool		libraryLoaded;
	bool		running;
const	char*		board_id_name (void);
	int32_t		selectedRate;
	std::complex<float>	*convBuffer;
	int16_t		convBufferSize;
	int16_t		convIndex;
	int16_t		mapTable_int   [4 * 512];
	float		mapTable_float [4 * 512];
	RingBuffer<std::complex<float>> *theBuffer;
	struct airspy_device* device;
	uint64_t 	serialNumber;
	char		serial[128];
    // callback buffer	
	int 		bs_;
	uint8_t 	*buffer;
	int		bl_;
static
	int		callback(airspy_transfer_t *);
	int		data_available (void *buf, int buf_size);
const	char *		getSerial (void);
	int		open (void);
};

