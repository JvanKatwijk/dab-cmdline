
	DAB-CMDLINE
========================================================================

DAB-CMDLINE is a DAB decoding program completely controlled through the command line.
The program is derived from the DAB-rpi and the sdr-j-DAB programs, however, no use is made of any GUI package.
It can be considered the GUI-less equivalent to the Qt-DAB program, that was also derived from its predecessors DAB-rpi and sdr-j-DAB programs.

===========================================================================
There is an obvious need - at least one that I have - to experiment with other (forms of) GUI(s) for a DAB handling program, using the same mechanism - preferably the same code - to handle the DAB data stream. That is why a choice was made to pack the full DAB handling as a library.

The library provides entries for the functionality through through some simple calls, while a few callback functions provide the communication back from the library.

An example of the use of the library is a simple command line based program, the sources of which are given in the directory "example". This program, when compiled and linked (a CMakeLists.txt file for use with cmake is available) supports the following Command line parameters:

 -B Band, selects the DAB band (default Band III),

 -M Mode, selects the DAB Mode (default Mode 1),

 -C the channel the default is 11C, the channel I am listening to mostly,

 -P the program name, a prefix suffices. For e.g. "Classic FM" it suffices to give "Classic". However, when passing on a non-unique prefix (e.g. "Radio" for "Radio Maria" and "Radio Veronica") the software will select one arbitrarily. Note that letter case is IMPORTANT is the current version. The names of the programs in the ensemble being received in the selected channel will be printed during recognition.

Important: If no program names are found, or if no match can be made between the program name and the list of program names, the program has no other choice than to halt, what it does.

 -G the gain to be applied on the device, a value in the range from 1 .. 100.
The value will be translated to an acceptable value for the device. In case the gain is table driven, as in the case of a dabstick, a value of e.g. 75 is translated into the element on three quarters of the table (basic assumption is that the table elements are more or less linear). For e.g. the airspy the values are mapped upon the range 0 .. 21 of the sensitivity slider. 
For e.g. the sdrplay, it is simple since there are 101 attenuation values. Setting the gain to N, implies setting the attenuation to 101 - N.

 -A the output channel, again as with the program name, a prefix of the name suffices. As with the programs, the names of the sound channels identified will be printed. Note, however, that in Linux not all all names appearing on the namelist are useful,
some of them will just not work, a well known  issue with the combination portaudio/alsa under Linux. 
Important: If a name is selected for a channel that cannot be opened the program will try to open the default output device.

For each of the parameters there is a default, i.e., if the command

	./linux/dab-cmdline
	
is given, the assumptions are 

 1) the Mode is "1",
 
 2) the band is "BAND III", that the channel selected is "11C",
 
 3) the program we are looking for is "Classic FM", and 
 
 4) the device to send the output to is "default". 
 
Note again, that the choice for the input device was fixed when creating the dab-library.

An example of a full specification of the command line is

	./linux/dab-cmdline -M 1 -B "BAND III" -C 12C -P "Radio 4" -G 80 -A default
	
============================================================================
The library can be created by adapting the CMakeLists.txt file in the dab-library directory and running

	mkdir build; cd build; cmake ..; make; make install
	IMPORTANT: YOU NEED C++11 SUPPORT FOR THIS

Note that - to keep things simple - the supported device, i.e. one of dabstick, airspy or sdrplay, is "compiled in" the library, so do not forget to select the device by adapting the CMakeLists.txt file before running the sequence mentioned above.

============================================================================
Libraries (together with the "development" or ".h" files) needed for creating the library are

	libfaad

	libfftw3f

	libusb-1.0

	zlib

and - obviously, the libraries for the device included.

Libraries for the example program are further:

	portaudio, version 19.xxx rather than 18.xxx that is installed default on some debian based systems;

	libsamplerate


==============================================================================================================
	The API

The API is the API of the dab library, it is a simple API with only a few functions. It is advised to look at the example program (especially the "main" program) to see how the API is used.

The API has three elements,

 1) typedefinition of the dabBand
 
 2) the type specifications of the callback functions,
 
 3) a specification of the functions callable in the API.


	enum dabBand {
	   BAND_III	= 0,
	   L_BAND	= 1
	};



	T H E   C A L L B A C K  F U N C T I O N S 	
	

The ensemble  - when discovered in the selected channel - is presented as a list of strings. That list is handed over to the user of the library by a user defined callback function. The boolean parameter in this function tells whether or not an ensemble was found. If no ensemble was found, it is (almost) certain that there is no decent signal.

The type of the callback function providing the program names as appearing in the ensemble, should be conformant to
 
	typedef void (*cb_ensemble_t)(std::list<std::string>, bool);

 Note that this function is *required* to be provided for,

 The resulting audio (PCM) samples - if any - are returned as pairs of 16 bit integers, with the length (in items), and the baudrate as other parameters. The PCM samples are passed on by the library to the user through a user defined callback function.
 The type of the callback function should be conformant to

	typedef void (*cb_audio_t)(int16_t *, int, int);

 if a NULL is provided as callback function, no data will be transferred.

 The dynamic labelvalue - if any - is passed through a user callback function, whose type is to be conformant to
 
	typedef void (*cb_data_t)(std::string);

 if a NULL is provided, no data will be transferred.

Some technical data of the selected program is passed through a callback function, whose type is to be conformant to

	typedef void (*cb_programdata_t)(int16_t,	// start address of the data for the selected program
	                                 int16_t,	// length in terms of CU's
	                                 int16_t,	// subchId
	                                 int16_t,	// protection
	                                 int16_t	// bitRate
	                                );

 if a NULL is provided, no data will be transferred.

	A P I - F U N C T I O N S
---------------------------------------------------------------------
 The initialization function takes as parameters the immutable system are
 
  1) the dabMode is just one of 1, 2 or 4 (Mode 3 is not supported),
  
  2) the dabBand, see the type above,
  
  3) the callback function for the sound handling or NULL if no sound output is required,
  
  4) the callback for handling the dynamic label or NULL if no sound output is required.

Note that by creating a dab-library, you already selected and a device, so the handler software for the device is part of the library.
The initialization function returns a non-NULL handle when the device could be opened for delivery input, otherwise it returns NULL.
 
	void	*dab_initialize	(uint8_t,	// dab Mode
	                         dabBand,	// Band
	                         cb_audio_t,	// callback for sound output
	                         cb_data_t	// callback for dynamic labels
	                         );
	
The return valie, i.e. the handle, is used to identify the library instance in the other functions defined in the API.
  
The gain of the device can be set and changed to a value  in the range 0 .. 100 using the function dab_Gain. The parameter value is mapped upon an appropriate value for the device
  
	void	dab_Gain	(void *handle, uint16_t);	

 The function dab_Channel maps the name of the channel onto a frequency for the device and prepares the device for action.
 If the software was already running for another channel, then the thread running the software will be halted first.
 
     bool	dab_Channel	(void *handle, std::string);

The function dab_run will start a separate thread, running the dab decoding software at the selected channel. If after some time, DAB data, i.e. an ensemble, is found, then the function passed as callback is called with the boolean parameter set to true, and the std::list of strings, representing the names of the programs in that ensemble. If no data was found, the boolean parameter is set to false, and the list is empty. 
 
 Note that the thread executing the dab decoding will continue to run.
 
     void	dab_run		(void *handle, cb_ensemble_t);

 With dab_Service, the user may - finally - select a program to be decoded. This - obviously only makes sense when there are programs and "dab_run" is still active. The name of the program may be a prefix of the real name, however, letter case is important.
 
     bool	dab_Service	(void *handle, std::string, cb_programdata_t);

The function stop will stop the running of the thread that is executing the dab decoding software

     void	dab_stop	(void *handle);

 The exit function will close down the library software and will set the handle, the address of which is passed as parameter, to NULL.
 
     void	dab_exit	(void **handle);


=============================================================================================

	The use of Python as GUI
	
Current experiments are being done to interface the library to Python 3. A wrapper for the API is to be found
in the file dab-python.cpp and a simple Python program was written to control the library through this
interface and to process the resulting sound samples.

In order to include the wrapper in the library, one has to uncomment PYTHON in the CMakeLists.txt file (and adapt the path name(s) in the CMakeLists.txt file.

The results are as yet not completely satisfactory: the current experiment uses the soundDevice.py software,
and while there is sound, there is too much stuttering to enjoy the result. The stuttering is probably due to the pretty large amounts of callbacks that are to be executed and the in-between buffering that is done between the callback from the dab-library and the callback from the soundDevice software.

===========================================================================

	Copyright (C)  2016, 2017
	Jan van Katwijk (J.vanKatwijk@gmail.com)
	Lazy Chair Programming

The dab-library software is made available under the GPL-2.0.
All SDR-J software, among which dab-library is one - is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 	GNU General Public License for more details.

