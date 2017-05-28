
DAB library and dab_cmdline

======================================================================
T H I S  I S  W O R K  I N  P R O G R E S S 

=======================================================================

DAB library is - as the name suggests - a library for decoding
a digitized DAB stream. 

There is an obvious need - at least felt by me - to experiment with other (forms of) GUI(s) for a DAB handling program, using the same mechanism - preferably the same code - to handle the DAB data stream. That is why a choice was made to pack the full DAB handling as a library. 

The library provides entries for the functionality through some simple calls, while a few callback functions provide the communication back from the library to the gui.

To show the use of the library, several example programs were developed.

	- example-1 and example-2 are command line DAB programs.
	  While example-1 uses the functionality implemented as
	  separate library, example-2 uses the sources directly.

	- simpleDab shows the use of the library when handled
	  from with a Qt GUI.

	- python-example contains an example program in python to use (an
	  extended form of) the library

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
the user program provides some functions to the library for getting samples.
============================================================================
Libraries (together with the "development" or ".h" files) needed for creating the library are

	libfaad
	libfftw3f
	libusb-1.0
	zlib


Libraries for the example program are further:

	portaudio, version 19.xxx rather than 18.xxx that is installed default on some debian based systems;
	libsamplerate

	library for the selected devices

============================================================================
Building the example programs
----------------------------------------------------------------------------

For both example-1 and example-2 program, there is a CMakeLists.txt file

example-1 is - obviously - the simplest one, since it expects a
library to be available to which it links

example-2 has the same look and the same functionality. 

For "simpleDab" one uses qt-make, there is a ".pro" file

For python-example read the README file in the python-example directory

=============================================================================

Command-line Parameters for the C++ versions
-----------------------------------------------------------------------

The C++ command line programs can be compiled using cmake. Of course, the dab library (see a next section) should have been installed. Libraries that are further needed are libsamplerate and portaudio.
The sequence to create the executable then is (after changing the working directory to the directory "example")

	mkdir build
	cd build
	cmake ..
	make

The program accepts the following command line parameters:

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
For e.g. the sdrplay, it is simple since there are 101 attenuation values. Setting the gain to N, implies setting the attenuation to 101 - N.

	-A the output channel
again as with the program name, a prefix of the name suffices. As with the programs, the names of the sound channels identified will be printed. Note, however, that in Linux not all all names appearing on the namelist are useful,
some of them will just not work, a well known  issue with the combination portaudio/alsa under Linux. 
Important: If a name is selected for a channel that cannot be opened the program will try to open the default output device.

For each of the parameters there is a default, i.e., if the command

	./linux/dab-cmdline
	
is given, the assumptions are 

1. the Mode is "1",
2. the band is "BAND III", that the channel selected is "11C",
3. the program we are looking for is "Classic FM", and 
4. the device to send the output to is "default". 
 
Note again, that the choice for the input device was fixed when creating the dab-library.

An example of a full specification of the command line is

	./linux/dab-cmdline -M 1 -B "BAND III" -C 12C -P "Radio 4" -G 80 -A default
The program - when started - will try to identify a DAB datastream in the selected channel (here channel 12C). If a valid DAB datastream is found, the names of the programs in the ensemble will be printed. If - after a given amount of time - no ensemble could be found, execution will halt, if the ensemble was identified, the data for the selected program (here Radio 4) will be decoded.

=========================================================================

simpleDAB

the simpleDAB directory contains the files for a simplified Qt GUI.
It binds to the dab-library, and can be created by qmake/make. No attempts
is made to create a CmakeLists.txt file, since the program is merely
an example to demonstrate the use of the library in a Qt context.
========================================================================

The API
-------------------------------------------------------------------------

The API specification, in dab-api.h, contains a specification of the
types for the callback functions and a specification for the real API functions.
===============================================================================
	
	Copyright (C)  2016, 2017
	Jan van Katwijk (J.vanKatwijk@gmail.com)
	Lazy Chair Programming

The dab-library software is made available under the GPL-2.0.
All SDR-J software, among which dab-library is one - is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 	GNU General Public License for more details.

