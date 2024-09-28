
DAB COMMAND LINE and DAB LIBRARY

---------------------------------------------------------------------
Introduction
---------------------------------------------------------------------

The DAB library provides entries for the functionality
to handle DAB/DAB+ through some simple calls.
A few callback functions provide the communication back from the
library to the caller.
The library interface is given in dab-api.h

Since I was unhappy with passing lots of individual callback functions
through the whole of the computing chain, I made (am making) a
change to the API and the dabInit function in the API.

Until recently one had to specify all individual callback functions
as parameter to the dabInit function.

The change is that now a struct "API_struct" is defined in the API,
the fields of which are the references to the different callback functions
such that the call to dabInit is basically simplified to

	theRadio        = dabInit (theDevice,
                                   &interface,
                                   nullptr,             // no spectrum shown
                                   nullptr,             // no constellations
                                   nullptr              // Ctx
                                  );

Examples 1,  2, 3, 4, 5 and 6 are adapted. Other examples - apart from the
python example are deleted.

The obvious advantage is that adding a callback function for a specific
purpose now does not change all of the intermediate functions in tbhe library,
basically opne only modifies

	a. the structure
	b. the main program for filling the structure and adding a handler
	c. the affected function in the library

New callback functions are

	a. the tii data 
	    typedef void (*tii_data_t)(int);
	   where the int is encoded as mainId << 8 + subInt

	b. the time 
	   typedef void    (*theTime_t)(std::string, void *);
	   where the time (hours::minutes) is passed as string

The handling of motdata is changed. In the previous version, slides
were - automatically - written to a file. In the current version, the
parameter profile of the motdata handling function is extended, the
motdata is passed on as uint8_t array, the size of the data is passed,
the name of the slides - as derived from the DAB data - is passed on.
See the dab-api for details


-----------------------------------------------------------------------
Disclaimer
-----------------------------------------------------------------------

The software is provided as is, it is available under the GPL-V2,
the examples might or might not work.
As far as I can see they work on my Linux-x86 box, no garantees are
given that the library software or the examples will work on
Windows or any other system.

THE EXAMPLES ARE NOT INTENDED TO BE FULL SOLUTIONS, BUT MERELY
THERE TO GIVE YOU AN IDEA HOW TO USE THE LIBRARY OR ITS SOURCES.
MODIFICATIONS IN THE LIBRARY ARE NOT ALWAYS TESTED ON ALL EXAMPLES,
PLEASE CORRECT THEM YOURSELF IF YOU (THINK YOU) FIND AN
ERROR BEFORE ASKING ME.

----------------------------------------------------------------------
The C (C++) example programs
------------------------------------------------------------------------

A number of example programs is included in the source tree, they are meant to
give an idea on how to use the library code, either as library or
as "built-in" sources. They might or might not work.

Note that the library depends on a device, but does not include that device.
The main program is responsible for ensuring that something is available
to deliver input samples and something is available for handling the output.


Invocation of the example programs, with some parameters specified, is
something like
     
	dab-sdrplay-x -M 1 -B "BAND III" -C 12C -P "Radio 4" -G 80 -A default

In this case, the example program was built with the SDRplay as device (other
possibilities are DABsticks, AIRspy devices, HACKRF and Lime SDR devices (and some
of the example programs can be configured to take file input)).
Furthermore, the example program will set the tuner to Band III, channel 12C,
will select the service "Radio 4". The Gain of the tuner is set to 80 (on 
a scale from 1 .. 100), the main program will select "default" as
audio output device.
The Library code will be set to interpret the input as being of Mode 1.

![example 3 with sdrplay input](/example-3.png?raw=true)

----------------------------------------------------------------------
Supported devices
----------------------------------------------------------------------

	- SDRplay RSP's (using 2.13 lib or 3.06 lib)
	- AIRspy
	- RTLSDR based devices
	- HACKRF (only example programs 2, 3, 4)
	- LimeSDR (only example programs 2, 3 and 4)
	- Pluto (only example 2)

and of course fileinput of ".raw" and ".sdr" files is supported, as
well as input through the rtl_tcp driver.


--------------------------------------------------------------------------
The examples
---------------------------------------------------------------------------

The examples are basically simple variations on a single theme,
example 2 is the basic one, others are derived.

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
	  and the AAC frames (in case of DAB+) are just emitted
	  into a file or to stdout. A flag "-f XXXX" to the command
	  line of this example, if specified, output is written to
	  the specified file, otherwise to stdout.
	  The output can be processed by e.g. VLC.
	  (Note that the AAC frames have 960 rather than 1024 samples,
	   not all audio programs are capable of handling these).

	- example 5 is a small experimental extension to example 2,
	  It contains a simple "keyboard listener", that will react
	  on entering a stroke on the return key. It will cause the
	  "next" (audio) service to be selected.
	  In example 5 there is support for tdc packet handling,
	  and - thanks to stefan Juhl - support for FEC protected packets.

	- example 6 is an experimental version where stdin is
	  used as input device (and the command line parameters are
	  adapted to that)

	- the python example seems to work, the cmake file now expects
	  python3.9 (it should work with other versions as well, adapt the
	  CMakeLists.tct file)

	- the scanner example is what the name suggests, it scans the band
	  and shows the content of the channels that carry (detectable)
	  DAB data

	For all examples it holds that NO garantee is
	given on their functioning, feel free to improve.

-------------------------------------------------------------------------------
A simple DAB scanner
-------------------------------------------------------------------------------

Next to these examples, a simple dab-scanner was made, an example program
that just scans all channels of the given band (BAND III by default)
and collects and emits data about the ensembles and services encountered.
Output can be sent to a file - ASCII - that can be interpreted
by Libre Office Calc or similar programs.

The dab-scanner supports rtlsdr, sdrplay (with support for the 2.13 lib and the 3.XX libraries), airspy, hackrf, and lime sdr.

![dab scanner with sdrplay input](/dab-scanner/dab-scanner.png?raw=true)

--------------------------------------------------------------------------
An example: installing example 2
--------------------------------------------------------------------------

As with other sdr programs, a number of libraries is to be installed.
Assuming that the program is to be compiled in a Debian based system
(e.g. Ubuntu)

	sudo apt-get update
	sudo apt-get install git cmake
	sudo apt-get install build-essential g++
	sudo apt-get install pkg-config
	sudo apt-get install libsndfile1-dev
	sudo apt-get install libfftw3-dev portaudio19-dev
	sudo apt-get install libfaad-dev zlib1g-dev 
	sudo apt-get install libusb-1.0-0-dev mesa-common-dev
	sudo apt-get install libgl1-mesa-dev
	sudo apt-get install libsamplerate0-dev

Installing the library for the SDRplay requires downloading the library
from the "www.SDRplay.com" site.

For pluto the "libiio-dev" has to be installed. Note that on "old"
Ubuntu versions, e.g. 16.04, only an old, not yet complete, version
of the library is available.

Installing the library for the RTLSDR can be done by executing the 
following lines

	git clone git://git.osmocom.org/rtl-sdr.git
	cd rtl-sdr/
	mkdir build
	cd build
	cmake ../ -DINSTALL_UDEV_RULES=ON -DDETACH_KERNEL_DRIVER=ON
	make
	sudo make install
	sudo ldconfig
	cd ..
	rm -rf build
	cd ..

Installing the library for the AIRspy can be done by executing the
following lines

	wget https://github.com/airspy/host/archive/master.zip
	unzip master.zip
	cd airspyone_host-master
	mkdir build
	cd build
	cmake ../ -DINSTALL_UDEV_RULES=ON
	make
	sudo make install
	sudo ldconfig
	cd ..
	rm -rf build
	cd ..

Once the libraries are installed, creating an executable for example 2
is straightforward

	cd dab-cmdline
	cd example-2
	mkdir build
	cd build
	cmake .. -DXXX=ON (replace XXX by the name of the device)
	make
	sudo make install
	cd ../../..

The resulting executable is installed in "/usr/local/bin"

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

	"dabReset_msc" will terminate operation of the handler(s) for the
	currently selected service


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
directory.


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
	Lazy Chair Computing

The dab-library software is made available under the GPL-2.0. The dab-library uses a number of GPL-ed libraries, all rigfhts gratefully acknowledged.
All SDR-J software, among which dab-library is one - is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 	GNU General Public License for more details.

