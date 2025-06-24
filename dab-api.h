#
/*
 *    Copyright (C) 2016, 2017, 2018
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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

#pragma once
#include	<stdio.h>
#include	<stdint.h>
#include	<string>
#include	<complex>
#include	"ringbuffer.h"

#if defined (__GNUC__) && (__GNUC__ >= 4)
#define DAB_API __attribute__((visibility("default")))
#elif defined (_MSC_VER)
#ifdef DAB_API_EXPORT
#define DAB_API __declspec(dllexport)
#elif DAB_API_STATIC
#define DAB_API
#else if
#define DAB_API __declspec(dllimport)
#endif
#else
#define DAB_API
#endif

//	Experimental API for controlling the dab software library
//
//	Version 4.0
//	Examples of the use of the DAB-API library are found in the
//	directories
//	a. C++ Example, which gives a simple command line interface to
//	   run DAB


#include	<stdint.h>
#include	"device-handler.h"
#include	"dab-constants.h"
//
//
//	This struct (a pointer to) is returned by callbacks of the type
//	programdata_t. It contains parameters, describing the service.
typedef struct {
	bool	defined;
	std::string	serviceName;
	std::string	shortName;
	uint32_t	SId;
	int16_t subchId;
	int16_t	startAddr;
	bool	shortForm;	// false EEP long form
	int16_t	protLevel;	// 
	int16_t DSCTy;
	int16_t	length;
	int16_t	bitRate;
	int16_t	FEC_scheme;
	int16_t	DGflag;
	int16_t	packetAddress;
	int16_t	appType;
	bool	is_madePublic;
} packetdata;

//
typedef	struct {
	bool	defined;
	std::string serviceName;
	std::string shortName;
	uint32_t	SId;
	int16_t		programType;
	
	int16_t	subchId;
	int16_t	startAddr;
	bool	shortForm;
	int16_t	protLevel;
	int16_t	length;
	int16_t	bitRate;
	int16_t	ASCTy;
	int16_t	language;
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
	typedef void (*name_of_ensemble_t)(const std::string &, int32_t, void *);
//
//	Each programname in the ensemble is sent once
	typedef	void (*serviceName_t)(const std::string &,
	                                  int32_t, uint16_t, void *);
//
//	thefib sends the time as pair of integers
	typedef void	(*theTime_t)(int hours, int minutes, void *);
	
//	after selecting an audio program, the audiooutput, packed
//	as PCM data (always two channels) is sent back
	typedef void (*audioOut_t)(int16_t *,		// buffer
	                           int,			// size
	                           int,			// samplerate
	                           bool,		// stereo
	                           void *);
//
//	dynamic label data, embedded in the audio stream, is sent as string
	typedef void (*dataOut_t)(const char *, void *);
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
//	The data of MOT pictures - i.e. slides encoded in the
//	Program Associated data is passed on as uint8_t data.
	typedef void (*motdata_t)(uint8_t *, int,
	                          const char *, int, void *);
//
//	tii data - if available, the tii data is passed on as a single
//	integer
	typedef void (*tii_data_t)(tiiData *, void *);

/////////////////////////////////////////////////////////////////////////
//
//	The API functions
extern "C" {
//	dabInit is called first, with a valid deviceHandler and a valid
//	Mode.
//	The parameters "spectrumBuffer" and "iqBuffer" will contain
//	-- if no NULL parameters are passed -- data to compute a
//	spectrumbuffer	and a constellation diagram.
//
//	The other parameters are as described above. For each of them a NULL
//	can be passed as parameter, with the expected result
//
typedef struct {
	uint8_t		dabMode;
	int16_t		thresholdValue;
	syncsignal_t	syncsignal_Handler;
	systemdata_t	systemdata_Handler;
	name_of_ensemble_t	name_of_ensemble;
	serviceName_t	serviceName;
	fib_quality_t	fib_quality_Handler;
	audioOut_t	audioOut_Handler;
	dataOut_t	dataOut_Handler;
	bytesOut_t	bytesOut_Handler;
	programdata_t	programdata_Handler;
	programQuality_t    program_quality_Handler;
	motdata_t	motdata_Handler;
	tii_data_t	tii_data_Handler;
	theTime_t	timeHandler;
} API_struct;

void DAB_API	*dabInit   (deviceHandler       *,
	                    API_struct		*,
	                    RingBuffer<std::complex<float>> *spectrumBuffer,
	                    RingBuffer<std::complex<float>> *iqBuffer,
	                    void                *userData);

//	dabExit cleans up the library on termination
void DAB_API	dabExit		(void *);
//
//	the actual processing starts with calling startProcessing,
//	note that the input device needs to be started separately
void DAB_API	dabStartProcessing (void *);
//
//	dabReset is as the name suggests for resetting the state of the library
void DAB_API	dabReset	(void *);
//
//	dabStop will stop operation of the functions in the library
void DAB_API	dabStop		(void *);
//
//	dabReset_msc will terminate the operation of active audio and/or data
//	handlers (there may be more than one active!).
//	If selecting a service (or services),
//	normal operation is to call first
//	on dabReset_msc, and then call set_xxxChannel for
//	the requested services
void DAB_API	dabReset_msc		(void *);
//
//	is_audioService will return true id the main service with the
//	name is an audioservice
bool DAB_API	is_audioService		(void *, const std::string &);
//
//	is_dataService will return true id the main service with the
//	name is a dataservice
bool DAB_API	is_dataService		(void *, const std::string &);
//
//	dataforAudioService will search for the audiodata of the i-th
//	(sub)service with the name as given. If no such service exists,
//	the "defined" bit in the struct will be set to false;
void DAB_API	dataforAudioService	(void *, const std::string &,
	                                            audiodata &, int);
//
//	dataforDataService will search for the packetdata of the i-th
//	(sub)service with the name as given. If no such service exists,
//	the "defined" bit in the struct will be set to false;
void DAB_API	dataforDataService	(void *, const std::string &,
	                                           packetdata &, int);
//
//	set-audioChannel will add - if properly defined - a handler
//	for handling the audiodata as described in the parameter
//	to the list of active handlers
void DAB_API	set_audioChannel	(void *, audiodata &);
//
//	set-dataChannel will add - if properly defined - a handler
//	for handling the packetdata as described in the parameter
//	to the list of active handlers
void DAB_API	set_dataChannel		(void *, packetdata &);
//
//	mapping from a name to a Service identifier is done 
int32_t DAB_API dab_getSId		(void *, const std::string &);
//
//	and the other way around, mapping the service identifier to a name
std::string DAB_API	dab_getserviceName	(void *, uint32_t /* SId */);
}
//
//	extract the name of the ensemble
std::string DAB_API	get_ensembleName	(void *);

