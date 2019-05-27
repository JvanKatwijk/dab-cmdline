
DAB COMMAND LINE and DAB LIBRARY

---------------------------------------------------------------------
Introduction
---------------------------------------------------------------------

The DAB library provides entries for the functionality
to handle DAB/DAB+ through some simple calls.
A few callback functions provide the communication back from the
library to the caller.
The library interface is given in dab-api.h

----------------------------------------------------------------------
The C (C++) example programs
------------------------------------------------------------------------

A number of example programs is included in the source tree, they are meant to
give an idea on how to use the library code, either as library or
as "built-in" sources.

Note that the library depends on a device, but does not include that device.
The main program is responsible for ensuring that something is available
to deliver input samples and something is available for handling the output.

THE EXAMPLES ARE NOT INTENDED TO BE FULL SOLUTIONS, BUT MERELY THERE TO GIVE YOU AN 
IDEA HOW TO USE THE LIBRARY OR ITS SOURCES. MODIFICATIONS IN THE LIBRARY ARE NOT ALWAYS
TESTED ON ALL EXAMPLES, PLEASE CORRECT THEM YOURSELF IF YOU (THINK YOU)
FIND AN ERROR BEFORE ASKING ME.

Invocation of the example programs, with some parameters specified, is
something like
     
	dab-sdrplay-x -M 1 -B "BAND III" -C 12C -P "Radio 4" -G 80 -A default

In this case, the example program was built with the SDRplay as device (other
possibilities are DABsticks, AIRspy devices and HACKRF devices (and some
of the example programs can be configured to take file input).
Furthermore, the example program will set the tuner to Band III, channel 12C,
will select the service "Radio 4". The Gain of the tuner is set to 80 (on 
a scale from 1 .. 100), the main program will select "default" as
audio output device.
The Library code will be set to interpret the input as being of Mode 1.

![example 3 with sdrplay input](/example-3.png?raw=true)

The examples 1 to 7 are basically simple variations on a single theme:

	- example 1 is the example where the main program is linked to
	  a precompiled shared library, 
	  i.e. the DAB library should be pre-installed

	- example 2 has the same functionality as example 1, the sources
          of the library are "compiled-in", however.

	- example 3 has the same functionality as example 2, and here
	  the library sources are "compiled in" as well. However, the
	  PCM samples are being sent out to stdout.
	  One might use one of the available programs to make the sound
	  audible
	  dab-example-3 .... | aplay -r 48000 -f S16_LE -t raw -c 2

	- example 4 has the sample functionality as examples 2 and 3, and
	  here the library sources are "compiled in" as well. However,
	  no sound decoding takes place. The MP2 frames (in case of DAB)
	  or the AAC frames (in case of DAB+) are just emitted through stdout.
	  (Note that the AAC frames have 960 rather than 1024 samples)

	- example 5 is a small experimental extension to example 2,
	  It contains a simple "keyboard listener", that will react
	  on entering a stroke on the return key. It will cause the
	  "next" (audio) service to be selected.

	- example 6 is an experimental version where control is
	  through an IP port. 
	  
	- example 7 is an experimental version where stdin is
	  used as input device (and the command line parameters are
	  adapted to that)

	- example 9 is a new project. It is derived from example 2
	  and it aims at retransmitting the audio from a selected
	  service as an FM signal, stereo and equipped with RDS
	  such that we can listen to the "warm" sound of old (tube)
	  radios. The program will emit the FM stereo samples either
	  to a file, or a tcp port.
	  "Normal use", i.e. audio output using the soundcard is also possible.

	  The rds text is derved from the dynamic label as emitted by
	  the dab service.

	  The sources for a simple "hackrf-server" are included in
	  the example-9 directory. This hackrf server will instruct
	  the hackrf device to transmit the incoming samples.

	  If you are not allowed to transmit (as most of us I assume),
	  a simple coax cable between the hackrf device and the "old"
	  radio to listen to will do.

	- in a separate project a variant of the DAB library is used to create 
	  a DAB server program, running as a service on an RPI 2/3 under Stretch and
	  being controlled by an android app (which is part of the development)

----------------------------------------------------------------------------
Example 10
----------------------------------------------------------------------------

Example 10 is contributed by Hayati Ayguen and - when run - gives some
information on the processing of a.o the TII. Compiling it will require
definition of some constants.

-------------------------------------------------------------------------------
A DAB scanner
-------------------------------------------------------------------------------

Next to these examples, a simple dab-scanner was made, an example program
that just scans all channels of the given band (BAND III by default)
and collects and emits data about the ensembles and services encountered.
Output can be sent to a file - ASCII - that can be interpreted
by Libre Office Calc or similar programs.


![dab scanner with sdrplay input](/dab-scanner/dab-scanner.png?raw=true)

 
----------------------------------------------------------------------------
Building an executable
----------------------------------------------------------------------------

For each of the programs, a CMakeLists.txt file exists with which a
Makefile can be generated using Cmake.

The standard way to create an executable is

	cd X   (replace X by the appropriate name of the example)
	mkdir build
        cd build
        cmake .. -DXXX=ON
        make
        sudo make install

where XXX is one of the supported input devices, i.e. SDRPLAY, AIRSPY, HACKRF,
RTLSDR, WAVFILES (".sdr"), or RTL_TCP. The name of the generated executable
is dab-xxx-y for the examples 1 - 7, and dab-scanner-xxx for the dab-scanner
program, where xxx is the device name, and y the number
of the example to which the executable belongs.

The executable will be installed (make install) in /usr/local/bin, so yo need to have
permissions (you can obviously also just run the generated program
from the build directory).

------------------------------------------------------------------------
The API
-----------------------------------------------------------------------

The full API description is given in the file dab-api.h

Initialization of the library is by a call to "dabInit". The call returns
a pointer (type void *) to structures internal to the library.

	"dabInit" gets pointers to callback functions as parameter and returns
	a "Handle", to be used in the other functions


	"dabExit" cleans up all resource use of the library.

	"dabReset" cleans up resource use and restarts the library

	"dabStartProcessing" returns immediately after being called
	 but will have created a few threads running in the background.

As soon as an ensemble is recognized, a callback function will be
called. Recognition of a service within an ensemble also
leads to calling a callback function, with the service name as parameter.

	"is_audioService" can be used to enquire whether or not a service
	with a given name is a recognized audio service,

	"is_dataService" can be used to enquire whether or not a service
	with a given name is a recognized data service,

	"dataforAudioService" is the function with which the (relevant) data,
	describing an audio service with a given name is fetched.
	The function fills a structure of type "audiodata", 
	it contains a field "defined" telling whether or not the
	data is the structure is valid or not.
	Note that if the last parameter is a 0-value, the main service
	is looked for, otherwise, the i-th subservice.

	"dataforDataService" is the function with which the (relevant) data,
	describing a data service with a given name is fetched.
	The function fills a structure of type "packetdata", 
	it contains a field "defined" telling whether or not the
	data is the structure is valid or not.
	Note that if the last parameter is a 0-value, the main service
        is looked for, otherwise, the i-th subservice.

	"set-audioChannel", when provided with a structure of type "audiodata",
	with valid data, will open and add an audio stream.

	"set-dataChannel", when provided with a structure of type "packetdata",
	with valid data, will open and add a data stream.

	Note that there is no built-in limit on the amount of open streams,
	although from a practical point of view there may be limitations,
	i.e two audiostreams may compete for a single library.

	What might be useful is to enquire for subservices such as MOT
	when opening an audio stream.

	"dabReset_msc", when called, will stop all open handlers for services
	and subservices.


-----------------------------------------------------------------------
A note on the callback functions
-----------------------------------------------------------------------

The library (whether separate or compiled in) sends its data to the
main program using callbacks. These callbacks, the specification of
which is given in the file dab-api.h, are implemented here as
simple C functions. WHAT MUST BE NOTED IS THAT THESE FUNCTIONS ARE
EXECUTED IN THE THREAD OF THE CALLER, and while the library is built around
more than a single thread, it is wise to add some locking when extending
the callback functions.

========================================================================

Creating the library
------------------------------------------------------------------------------

The library can be created by - if needed - adapting the
`CMakeLists.txt` file in the dab-library/library directory and running

	mkdir build 
	cd build 
	cmake .. 
	make 
	sudo make install
	
from within the dab-library directory.

IMPORTANT: YOU NEED C++11 SUPPORT FOR THIS

Note that contrary to earlier versions, the "device" is NOT part of the library,
the user program has to provide some functions to the library for getting samples.
The interface can be found in the file "device-handler.h". 

===============================================================================

Libraries (together with the "development" or ".h" files) needed for creating the library are

	libfaad
	libfftw3f
	libusb-1.0
	zlib


============================================================================

For the python-example read the README file in the python-example directory.
HOWEVER: before running the example program one has to create an
ADAPTED library.
The CMakeLists.txt file for creating such an adapted library is in the python
directory. Note that the python example is not maintained and a little obsolete.


=============================================================================

Command-line Parameters for the C (C++) exmple programs
-----------------------------------------------------------------------

The programs accept the following command line parameters:

	-B Band
selects the DAB band (default Band III),

	-M Mode
selects the DAB Mode (default Mode 1),

	-C the channel
the default is 11C, the channel I am listening to mostly,

	-P the program name
a prefix suffices. For e.g. "Classic FM" it suffices to give "Classic". However, when passing on a non-unique prefix (e.g. "Radio" for "Radio Maria" and "Radio Veronica") the software will select one arbitrarily. Note that letter case is IMPORTANT is the current version. The names of the programs in the ensemble being received in the selected channel will be printed during recognition.

Important: If no program names are found, or if no match can be made between the program name and the list of program names, the program has no other choice than to halt, what it does.

	-G the gain 
to be applied on the device, a value in the range from 1 .. 100.
The value will be translated to an acceptable value for the device. In case the gain is table driven, as in the case of a dabstick, a value of e.g. 75 is translated into the element on three quarters of the table (basic assumption is that the table elements are more or less linear). For e.g. the Airspy the values are mapped upon the range 0 .. 21 of the sensitivity slider. 
Note that when using the rtl_tcp interface, this does not hold. The sound
setting is passed on to the server.

	-W waiting time
the maximum time to wait for valid data.
If no valid data is found within this period, execution of the program will
stop.

Note that in example-2 the -W is replaced by TWO parameters, a -d xxx indicating
the maximum amount of time to reach time synchronization (which is implicit
in the other examples) and a -D xxx parameter indicating the maximum amount of
time to get the signal "ensemble found".

	-A the output channel (example 1 and 2 only)
again as with the program name, a prefix of the name suffices. As with the programs, the names of the sound channels identified will be printed. Note, however, that in Linux not all all names appearing on the namelist are useful,
some of them will just not work, a well known  issue with the combination portaudio/alsa under Linux. 
Important: If a name is selected for a channel that cannot be opened the program will try to open the default output device.

	-O filename or "-" (example 2 only) 
The PCM samples of the sound output are stored in the file <filename>. If "-"
is specified as filename the output is just written to stdout.
This output then can be made audible by some other program.


	-H hostname (example 2, 3 and 4 only)
If rtl_tcp is selected as input device, the -H option allows selection
of a hostname. Default is "127.0.0.1".

	-I port (example 2, 3, and 4 only)
If rtl_tcp is selected as input device, the -I option allows selection
of a port. Default is 1234.

----------------------------------------------------------------------------
Interfaces
---------------------------------------------------------------------------

In order to use the library, the main program has to deal with
two interfaces, obviously the dab library, but also the device.

The device interface is specified in "./device-handler.cpp".

----------------------------------------------------------------------------
E X P E R I M E N T A L
----------------------------------------------------------------------------

One of the issues still to be resolved is the handling of data. As an
experiment a callback function was added that is called from within the
tdc handler. In example-2 a simple TCP server was added, one that just
writes out packaged tdc frames.
The package structure is : an 8 byte header followed by the frame data.
The header starts with a -1 0 -1 0 pattern, followed by a two byte length,
followed by a zero, followed by a 0 for frametype 0 and 0xFF for frametype 1.
Install the server by adding "-DSERVER" to the cmake command line.

A simple "reader" (client), using qt is included in the sources.

-------------------------------------------------------------------------
Copyrights
-------------------------------------------------------------------------
	
	Copyright (C)  2016, 2017, 2018
	Jan van Katwijk (J.vanKatwijk@gmail.com)
	Lazy Chair Programming

The dab-library software is made available under the GPL-2.0. The dab-library uses a number of GPL-ed libraries, all
rigfhts gratefully acknowledged.
All SDR-J software, among which dab-library is one - is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 	GNU General Public License for more details.

