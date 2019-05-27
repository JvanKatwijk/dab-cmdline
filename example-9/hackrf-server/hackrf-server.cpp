#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the hackrf transmitter
 *    hackrf transmitter is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    hackrf transmitter is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with hackrf transmitter; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	<numeric>
#include	<unistd.h>
#include	<complex>
#include	<vector>
#include	"hackrf-server.h"
#include	"hackrf-handler.h"

#include	<mutex>
/**
  */
	hackrf_server::
	       hackrf_server (int	vfoFrequency,
	                      int	inputRate,
	                      int	gain,
	                      int	transmissionRate,
	                      RingBuffer<uint8_t> *inputBuffer) {

	this	-> vfoFrequency		= vfoFrequency;
	this	-> gain			= gain;
	this	-> inputBuffer		= inputBuffer;
        this    -> upFactor		=
	                         transmissionRate / inputRate;
	if (upFactor > 1)
           this    -> upFilter		=
	                         new LowPassFIR (3,
	                                         inputRate / 2,
                                                 transmissionRate);

	theTransmitter			=
	                         new hackrfHandler  (vfoFrequency,
	                                             gain,
	                                             transmissionRate);
	if (theTransmitter == NULL) {
	   throw (22);
	}
	connected	= false;
}

	hackrf_server::~hackrf_server (void) {
	stop ();
	delete	theTransmitter;
	if (upFactor > 1)
	   delete	upFilter;
}

void	hackrf_server::stop	(void) {
	if (!running. load ())
	   return;
	running. store (false);
	theTransmitter	-> stop ();
	usleep (1000);
	threadHandle. join ();
}

void	hackrf_server::start	(void) {
	threadHandle	= std::thread (&hackrf_server::run, this);
	running. store (true);
}

#define	CHUNK_SIZE	256
void	hackrf_server::run	(void) {
char theData [CHUNK_SIZE * 4];
std::complex<float> buffer [CHUNK_SIZE * upFactor];
std::complex<float> sample;
int	fillerP		= 0;

	theTransmitter	-> start (vfoFrequency, gain);
	while (running. load ()) {
	   while (running. load () &&
	          inputBuffer -> GetRingBufferReadAvailable () < CHUNK_SIZE * 4)
	      usleep (1000);
	
	      int amount =
	           inputBuffer -> getDataFromBuffer (theData, CHUNK_SIZE * 4);
	      fillerP = 0;
	      for (int i = 0; i < CHUNK_SIZE; i ++) {
	         int16_t re = (theData [4 * i] << 8) |
	                              (uint8_t)(theData [4 * i + 1]);
	         int16_t im = (theData [4 * i + 2] << 8) |
	                              (uint8_t)(theData [4 * i + 3]);
	         sample = std::complex<float> (re / 32767.0, im / 32767.0);
	         if (upFactor > 1) 
	            buffer [fillerP++] = upFilter -> Pass (sample);
	         else
	            buffer [fillerP ++] = sample;

	         for (int j = 1; j < upFactor; j ++)
	            buffer [fillerP ++] =
	                  upFilter -> Pass (std::complex<float> (0, 0));
	      }

	      theTransmitter	-> passOn (buffer, fillerP);
	}
	theTransmitter	-> stop ();
}
