
	DAB-CMDLINE
========================================================================

DAB-CMDLINE is a DAB decoding program that is completely controlled
through the command line.
The program is derived from the DAB-rpi and the sdr-j-dab programs,
however, no use is made of any GUI package.

===========================================================================
The  basic idea is that the full dab handling will be done by
a piece of program, that is reusable, i.e. built as a library.
The rationale being that it is now easy to create implementations
with different GUIs, even in different languages.

Communication with the library is through some calls and a few
callback functions provide the output, described in the api document.

The library can be created by adapting the CMakeLists.txt file
in the dab-library directory and running

	mkdir build; cd build; cmake ..; make; make install

Note that - to keep things simple - the supported device, i.e. one of
dabstick, airspy or sdrplay, is "compiled in" the library.

============================================================================
As an example, a simple command line based program is given in
the directory "example".
This program, when compiled and linked, for which purpose there is
also a CMakeLists.txt file - supports the following Command line parameters

 -B Band, selects the DAB band (default Band III),

 -M Mode, selects the DAB Mode (default Mode 1),

 -C the channel the default is 11C, the channel I am listening to mostly,

 -P the program name, a prefix suffices. For e.g. "Classic FM" it suffices to give "Classic". However, when passing on a non-unique prefix (e.g. "radio" for "Radio Maria" and "Radio Veronica") the software will select one arbitrarily. Note that letter case is IMPORTANT. The names of the programs in the ensemble being received in the selected channel will be printed during recognition.

Important: If no program names are found, or if no match can be made between the program name and the list of program names, the program has no other choice than to halt, what it does.

 -G the gain to be applied on the device, a value in the range from 1 .. 100.
The value will be translated to an acceptable value for the device.
In case the gain is table driven, as in the case of a dabstick, a value of e.g. 75 is translated into the element on three quarters of the table (basic assumption is that the table elements are more or less linear). For e.g. the airspy the values are mapped upon the range 0 .. 21 of the sensitivity slider. 
For e.g. the sdrplay, it is simple since there are 101 attenuation values. Setting the gain to N, implies setting the attenuation to 101 - N.

 -A the output channel, again as with the program name, a prefix of the name suffices. As with the programs, the names of the sound channels identified will be printed. Note, however, that not all names appearing on the namelist are useful,
some of them will just not work, a well known  issue with the combination portaudio/alsa under Linux. 
Important: If a name is selected for a channel that cannot be opened the program will try to open the default output device.

Important:
_________

For each of the parameters there is a default.
I.e., if the command

	./linux/dab-cmdline
will assume that the Mode is "1",
that the band is "BAND III", that the channel selected is "11C",
that the program we are looking for is "Classic FM", and that the
device to send the output to is "default". (Note again, that the
choice for the input device was fixed when creating the dab-library.

Example

	./linux/dab-cmdline -M 1 -B "BAND III" -C 12C -P "Radio 4" -G 80 -A default
	
============================================================================

Libraries (together with the "development" or ".h" files) for creating the
library are

	libfaad

	libfftw3f

	libusb-1.0

	zlib

and - obviously, the libraries for the devices included.

Libraries for the example program are further:

	portaudio, version 19.xxx rather than 18.xxx that is
	installed default on some debian based systems;

	libsamplerate

===========================================================================
===========================================================================

	Copyright (C)  2016, 2017
	Jan van Katwijk (J.vanKatwijk@gmail.com)
	Lazy Chair Programming

	The dab-library software is made available under the GPL-2.0.
	SDR-J is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

