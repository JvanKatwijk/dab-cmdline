#
/*
 *    Copyright (C) 2016, 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is the API description of the DAB-library.
 *
 *    DAB-library is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB-library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB-library, if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef		__DAB_API__
#define		__DAB_API__
#include	<stdio.h>
#include	<stdint.h>
#include	<string>
#include	<complex>
#include	"ringbuffer.h"

//	Experimental API for controlling the dab software library
//
//	Version 2.0
//	Examples of the use of the DAB-API library are found in the
//	directories
//	a. C++ Example, which gives a simple command line interface to
//	   run DAB
//	b. python, which gives a python program implementing a simple
//	   command line interface to run DAB
//	c. simpleDab, which implements a Qt interface - similar to that
//	   of Qt-DAB - to the library


#include	<stdint.h>
#include	"device-handler.h"
//
//
//	This struct (a pointer to) is returned by callbacks of the type
//	programdata_t. It contains parameters, describing the service.
typedef struct {
	bool	defined;
	int16_t subchId;
	int16_t	startAddr;
	bool	shortForm;
	int16_t	protLevel;
	int16_t DSCTy;
	int16_t	length;
	int16_t	bitRate;
	int16_t	FEC_scheme;
	int16_t	DGflag;
	int16_t	packetAddress;
	int16_t	appType;
} packetdata;

//
//	currently not used
typedef	struct {
	bool	defined;
	int16_t	subchId;
	int16_t	startAddr;
	bool	shortForm;
	int16_t	protLevel;
	int16_t	length;
	int16_t	bitRate;
	int16_t	ASCTy;
	int16_t	language;
	int16_t	programType;
} audiodata;

//////////////////////// C A L L B A C K F U N C T I O N S ///////////////
//
//
//	A signal is sent as soon as the library knows that time
//	synchronization will be ok.
//	Especially, if the value sent is false, then it is (almost)
//	certain that no ensemble will be detected
	typedef void (*syncsignal_t)(bool, void *);
//
//	the systemdata is sent once per second with information
//	a. whether or not time synchronization is OK
//	b. the SNR, 
//	c. the computed frequency offset (in Hz)
	typedef	void (*systemdata_t)(bool, int16_t, int32_t, void *);
//
//	the fibQuality is sent regularly and indicates the percentage
//	of FIB packages that pass the CRC test
	typedef void (*fib_quality_t) (int16_t, void *);
//
//	the ensemblename is sent whenever the library detects the
//	name of the ensemble
	typedef void (*ensemblename_t)(std::string, int32_t, void *);
//
//	Each programname in the ensemble is sent once
	typedef	void (*programname_t)(std::string, int32_t, void *);
//
//	after selecting an audio program, the audiooutput, packed
//	as PCM data (always two channels) is sent back
	typedef void (*audioOut_t)(int16_t *,		// buffer
	                           int,			// size
	                           int,			// samplerate
	                           bool,		// stereo
	                           void *);
//
//	dynamic label data, embedded in the audio stream, is sent as string
	typedef void (*dataOut_t)(std::string, void *);
//
//
//	byte oriented data, emitted by various dataHandlers, is sent
//	as array of uint8_t values (packed bytes)
	typedef void (*bytesOut_t)(uint8_t *, int16_t, uint8_t, void *);

//	the quality of the DAB data is reflected in 1 number in case
//	of DAB, and 3 in case of DAB+,
//	the first number indicates the percentage of dab packages that
//	passes tests, and for DAB+ the percentage of valid DAB_ frames.
//	The second and third number are for DAB+: the second gives the
//	percentage of packages passing the Reed Solomon correction,
//	and the third number gives the percentage of valid AAC frames
	typedef void (*programQuality_t)(int16_t, int16_t, int16_t, void *);
//
//	After selecting a service, parameters of the selected program
//	are sent back.
	typedef void (*programdata_t)(audiodata *, void *);

//
//	MOT pictures - i.e. slides encoded in the Program Associated data
//	are stored in a file. Each time such a file is created, the
//	function registered as
	typedef void (*motdata_t)(std::string, int, void *);
//	is invoked (if not specified as NULL)
/////////////////////////////////////////////////////////////////////////
//
//	The API functions
extern "C" {
//	dabInit is called first, with a valid deviceHandler and a valid
//	Mode. The parameters "spectrumBuffer" and "iqBuffer" will contain
//	-- if no NULL parameters are passed -- the data to fill a spectrumbuffer
//	and a constellation diagram. There use can be seen in the example
//	program "simpleDab".
//
//	The other parameters are as described above. For each of them a NULL
//	can be passed as parameter, with the expected result.
//
//	The "spectrumBuffer" and the "iqBuffer" parameters provide
//	- as the name suggests - spectrumdata and data to show the
//	constellation diagram.
//	Look into the simpleDab example to see how they are/can be used.
void	*dabInit   (deviceHandler       *,
	            uint8_t             Mode,
	            RingBuffer<std::complex<float>> *spectrumBuffer,
	            RingBuffer<std::complex<float>> *iqBuffer,
	            syncsignal_t        syncsignalHandler,
	            systemdata_t        systemdataHandler,
	            ensemblename_t      ensemblenameHandler,
	            programname_t       programnamehandler,
	            fib_quality_t       fib_qualityHandler,
	            audioOut_t          audioOut_Handler,
	            dataOut_t           dataOut_Handler,
	            bytesOut_t		bytesOut,
	            programdata_t       programdataHandler,
	            programQuality_t    program_qualityHandler,
	            motdata_t		motdata_Handler,
	            void                *userData);

//	dabExit cleans up the library on termination
void	dabExit		(void *);
//
//	the actual processing starts with calling startProcessing,
//	note that the input device needs to be started separately
void	dabStartProcessing (void *);
//
void	dabReset	(void *);
void	dabStop		(void *);
//	reset is as the name suggests for resetting the state of the library
//
//	after selecting a name for a program, the service can be started
//	by calling dab_service. Note that it is assumed that an ensemble
//	was recognized. The function returns with a positive number
//	when successfull, otherwise it will
//	return -1 is the service cannot be found
//	return -2 if the type of service is not recognized
//	return -3 if the type of service is not audio
//	return -4 if the service is insufficiently defined
int16_t	dabService     (const char*, void *);
//
//	mapping from a name to a Service identifier is done 
int32_t dab_getSId      (const char*, void *);
//
//	and the other way around, mapping the service identifier to a name
std::string dab_getserviceName (int32_t, void *);
}

#endif

