#
/*
 *    Copyright (C) 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the main program of the DAB library
 *
 *    DAB library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"audiosink.h"
#include	<stdio.h>
/*
 */
	audioSink::audioSink	(int16_t latency,
	                         std::string soundChannel,
	                         bool *err):
	                                             audioBase () {
int32_t	i;
	this	-> latency	= latency;
	this	-> CardRate	= 48000;
	_O_Buffer		= new RingBuffer<float>(2 * 32768);
	portAudio		= false;
	writerRunning		= false;
	if (Pa_Initialize () != paNoError) {
	   fprintf (stderr, "Initializing Pa for output failed\n");
	   return;
	}

	portAudio	= true;
	fprintf (stderr, "Hostapis: %d\n", Pa_GetHostApiCount ());

	for (i = 0; i < Pa_GetHostApiCount (); i ++)
	   fprintf (stderr, "Api %d is %s\n", i, Pa_GetHostApiInfo (i) -> name);

	numofDevices	= Pa_GetDeviceCount ();
	outTable	= new int16_t [numofDevices + 1];
	for (i = 0; i < numofDevices; i ++)
	   outTable [i] = -1;
	ostream		= NULL;
	*err =  !selectDevice (soundChannel);
}

	audioSink::~audioSink	(void) {
	if ((ostream != NULL) && !Pa_IsStreamStopped (ostream)) {
	   paCallbackReturn = paAbort;
	   (void) Pa_AbortStream (ostream);
	   while (!Pa_IsStreamStopped (ostream))
	      Pa_Sleep (1);
	   writerRunning = false;
	}

	if (ostream != NULL)
	   Pa_CloseStream (ostream);

	if (portAudio)
	   Pa_Terminate ();

	delete	_O_Buffer;
	delete[] outTable;
}

bool	audioSink::selectDevice (const std::string soundChannel) {
PaError err;
int16_t	odev	= 0, i;

	fprintf (stderr, "selecting device %s\n", soundChannel. c_str ());

	for (i = 0; i <  numofDevices; i ++) {
	   const std::string so = 
	             outputChannelwithRate (i, CardRate);
	   if (so == std::string (""))
	      continue;
	   fprintf (stderr, "device %s seems available as %d\n",
	                                   so. c_str (), i);
	   if (so. find (soundChannel,0) != std::string::npos)
	      odev = i;
	}

	if (!isValidDevice (odev)) {
	   fprintf (stderr, "invalid device (%d) selected\n", odev);
	   return false;
	}

	if ((ostream != NULL) && !Pa_IsStreamStopped (ostream)) {
	   paCallbackReturn = paAbort;
	   (void) Pa_AbortStream (ostream);
	   while (!Pa_IsStreamStopped (ostream))
	      Pa_Sleep (1);
	   writerRunning = false;
	}

	if (ostream != NULL)
	   Pa_CloseStream (ostream);

	outputParameters. device		= odev;
	outputParameters. channelCount		= 2;
	outputParameters. sampleFormat		= paFloat32;
	outputParameters. suggestedLatency	= 
	                          Pa_GetDeviceInfo (odev) ->
	                                      defaultHighOutputLatency * latency;
	bufSize	= (int)((float)outputParameters. suggestedLatency);
	bufSize	= latency * 128;

	outputParameters. hostApiSpecificStreamInfo = NULL;
//
	fprintf (stderr, "Suggested size for outputbuffer = %d\n", bufSize);
	err = Pa_OpenStream (&ostream,
	                     NULL,
	                     &outputParameters,
	                     CardRate,
	                     bufSize,
	                     0,
	                     this	-> paCallback_o,
	                     this
	                    );

	if (err != paNoError) {
	   fprintf (stderr, "Open ostream error\n");
	   return false;
	}

	paCallbackReturn = paContinue;
	err = Pa_StartStream (ostream);
	if (err != paNoError) {
	   fprintf (stderr, "Open startstream error\n");
	   return false;
	}

	writerRunning	= true;
	return true;
}

void	audioSink::restart	(void) {
PaError err;

	if (!Pa_IsStreamStopped (ostream))
	   return;

	_O_Buffer	-> FlushRingBuffer ();
	paCallbackReturn = paContinue;
	err = Pa_StartStream (ostream);
	if (err == paNoError)
	   writerRunning	= true;
}

void	audioSink::stop	(void) {
	if (Pa_IsStreamStopped (ostream))
	   return;

	paCallbackReturn	= paAbort;
	(void)Pa_StopStream	(ostream);
	while (!Pa_IsStreamStopped (ostream))
	   Pa_Sleep (1);
	writerRunning		= false;
}
//
//	helper
bool	audioSink::OutputrateIsSupported (int16_t device, int32_t Rate) {
PaStreamParameters *outputParameters =
	           (PaStreamParameters *)alloca (sizeof (PaStreamParameters)); 

	outputParameters -> device		= device;
	outputParameters -> channelCount	= 2;	/* I and Q	*/
	outputParameters -> sampleFormat	= paFloat32;
	outputParameters -> suggestedLatency	= 0;
	outputParameters -> hostApiSpecificStreamInfo = NULL;

	return Pa_IsFormatSupported (NULL, outputParameters, Rate) ==
	                                          paFormatIsSupported;
}
/*
 * 	... and the callback
 */
int	audioSink::paCallback_o (
		const void*			inputBuffer,
                void*				outputBuffer,
		unsigned long			framesPerBuffer,
		const PaStreamCallbackTimeInfo	*timeInfo,
	        PaStreamCallbackFlags		statusFlags,
	        void				*userData) {
RingBuffer<float>	*outB;
float	*outp		= (float *)outputBuffer;
audioSink *ud		= reinterpret_cast <audioSink *>(userData);
uint32_t	actualSize;
uint32_t	i;
	(void)statusFlags;
	(void)inputBuffer;
	(void)timeInfo;
	if (ud -> paCallbackReturn == paContinue) {
	   outB = (reinterpret_cast <audioSink *> (userData)) -> _O_Buffer;
	   actualSize = outB -> getDataFromBuffer (outp, 2 * framesPerBuffer);
	   for (i = actualSize; i < 2 * framesPerBuffer; i ++)
	      outp [i] = 0;
	}

	return ud -> paCallbackReturn;
}

void	audioSink::audioOutput	(float *b, int32_t amount) {
	_O_Buffer	-> putDataIntoBuffer (b, 2 * amount);
}

const char *audioSink::outputChannelwithRate (int16_t ch, int32_t rate) {
const PaDeviceInfo *deviceInfo;

	if ((ch < 0) || (ch >= numofDevices))
	   return "";

	deviceInfo = Pa_GetDeviceInfo (ch);
	if (deviceInfo == NULL)
	   return "";
	if (deviceInfo -> maxOutputChannels <= 0)
	   return "";

	if (OutputrateIsSupported (ch, rate))
	   return (deviceInfo -> name);
	return "";
}

int16_t	audioSink::invalidDevice	(void) {
	return numofDevices + 128;
}

bool	audioSink::isValidDevice (int16_t dev) {
	return 0 <= dev && dev < numofDevices;
}

bool	audioSink::selectDefaultDevice (void) {
	return selectDevice ("default");
}

int32_t	audioSink::cardRate	(void) {
	return 48000;
}

int16_t	audioSink::numberofDevices	(void) {
	return numofDevices;
}

