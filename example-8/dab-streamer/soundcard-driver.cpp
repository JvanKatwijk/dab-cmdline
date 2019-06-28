#
/*
 *    Copyright (C) 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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

#include	"soundcard-driver.h"
#include	<stdio.h>
/*
 */
	soundcardDriver::soundcardDriver	(int16_t latency,
	                                         std::string soundChannel) {
int32_t	i;
	this	-> latency	= latency;
	this	-> CardRate	= 192000;
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

	fprintf (stderr, "looking for a device with a rate 192000\n");
	numofDevices	= Pa_GetDeviceCount ();
	outTable	= new int16_t [numofDevices + 1];
	for (i = 0; i < numofDevices; i ++)
	   outTable [i] = -1;
	ostream		= NULL;
	if (!selectDevice (soundChannel))
	   throw (21);
}

	soundcardDriver::~soundcardDriver	(void) {
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

bool	soundcardDriver::selectDevice (const std::string soundChannel) {
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

void	soundcardDriver::restart	(void) {
PaError err;

	if (!Pa_IsStreamStopped (ostream))
	   return;

	_O_Buffer	-> FlushRingBuffer ();
	paCallbackReturn = paContinue;
	err = Pa_StartStream (ostream);
	if (err == paNoError)
	   writerRunning	= true;
}

void	soundcardDriver::stop	(void) {
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
bool	soundcardDriver::OutputrateIsSupported (int16_t device, int32_t Rate) {
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
int	soundcardDriver::paCallback_o (
		const void*			inputBuffer,
                void*				outputBuffer,
		unsigned long			framesPerBuffer,
		const PaStreamCallbackTimeInfo	*timeInfo,
	        PaStreamCallbackFlags		statusFlags,
	        void				*userData) {
RingBuffer<float>	*outB;
float	*outp		= (float *)outputBuffer;
soundcardDriver *ud		= reinterpret_cast <soundcardDriver *>(userData);
uint32_t	actualSize;
uint32_t	i;
	(void)statusFlags;
	(void)inputBuffer;
	(void)timeInfo;
	if (ud -> paCallbackReturn == paContinue) {
	   outB = (reinterpret_cast <soundcardDriver *> (userData)) -> _O_Buffer;
	   actualSize = outB -> getDataFromBuffer (outp, 2 * framesPerBuffer);
	   for (i = actualSize; i < 2 * framesPerBuffer; i ++)
	      outp [i] = 0;
	}
	return ud -> paCallbackReturn;
}

void	soundcardDriver::sendSample	(std::complex<float> v) {
float temp [2];
	_O_Buffer	-> putDataIntoBuffer (temp, 2);
}

const char *soundcardDriver::outputChannelwithRate (int16_t ch, int32_t rate) {
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

int16_t	soundcardDriver::invalidDevice	(void) {
	return numofDevices + 128;
}

bool	soundcardDriver::isValidDevice (int16_t dev) {
	return 0 <= dev && dev < numofDevices;
}

bool	soundcardDriver::selectDefaultDevice (void) {
	return selectDevice ("default");
}

int32_t	soundcardDriver::cardRate	(void) {
	return 192000;
}

int16_t	soundcardDriver::numberofDevices	(void) {
	return numofDevices;
}

