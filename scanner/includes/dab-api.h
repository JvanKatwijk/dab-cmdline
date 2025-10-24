#
/*
 *    Copyright (C) 2025
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is the API description of the DAB-scanner.
 *
 *    DAB-scanner is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    DAB-scanner is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with DAB-scanner, if not, write to the Free Software
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

//	API for interfacing to the main program to the scanner functions
//
//	Version 1.0


#include	<stdint.h>
#include	"device-handler.h"
#include	"dab-constants.h"
//
//
//////////////////////// C A L L B A C K F U N C T I O N S ///////////////
//
//	A signal is sent as soon as the ofdm handler has seen that
//	synchronization will be ok.
	typedef void (*syncsignal_t)(bool, void *);
//
//	name of the ensemble
	typedef void (*name_of_ensemble_t)(const std::string &, int32_t, void *);
//
//	Each programname in the ensemble is sent once
	typedef	void (*serviceName_t)(const std::string &,
	                                  int32_t, uint16_t, void *);
//
//	thefib sends the time as pair of integers
	typedef void	(*theTime_t)(int hours, int minutes, void *);
	
//	tii data - if available, the tii data is passed on as a single
//      integer
        typedef void (*tii_data_t)(tiiData *, void *);


typedef struct {
        int16_t         thresholdValue;
        syncsignal_t    syncsignal_Handler;
	tii_data_t	tii_data_Handler;
        name_of_ensemble_t      name_of_ensemble;
        serviceName_t   serviceName;
        theTime_t       timeHandler;
} API_struct;
 

