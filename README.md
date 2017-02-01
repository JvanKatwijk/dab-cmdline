
DAB-CMDLINE

DAB-CMDLINE is a DAB decoding program that is completely controlled
through the command line.
The program is derived from the DAB-rpi and the sdr-j-dab programs,
however, no use is made of any Qt or qwt library.

The configuration is still pretty experimental, and may change almost daily.

Parameters can be set through the command line on starting the program:

-D device, selects the device (one of the configured ones, default a dabstick);

-B Band, selects the DAB band (default Band III),

-M Mode, selects the DAB Mode (default Mode 1),

-C the channel the default is 11C, the channel I am listening to mostly,

-P the program name, a prefix suffices. For e.g. "Classic FM" it suffices to give "Classic". However, when passing on a non-unique prefix (e.g. "radio" for "Radio Maria" and "Radio Veronica") the software will select one arbitrarily. Note that letter case is IMPORTANT. The names of the programs in the ensemble being received in the selected channel will be printed during recognition.

Important: If no program names are found, or if no match can be made between the
program name and the list of program names, the program has no other choice than halt.

-G the gain to be applied on the device, a value in the range from 1 .. 100.
The value will be translated to an acceptable value for the device.
In case the gain is
table driven, as in the case of a dabstick, a value of e.g. 75 is translated
into the element on three quarters of the table (basic assumption is that the
table elements are more or less linear). For e.g. the airspy the values are mapped upon the range 0 .. 21 of the sensitivity slider.
For e.g. the sdrplay, it is simple since there are 101 attenuation values.
Setting the gain to N, implies setting the attenuation to 101 - N.

-A the output channel, again as with the program name, a prefix of the name suffices. As with the programs, the names of the sound channels identified will be printed. Note, however, that not all names appearing on the namelist are useful,
some of them will just not work, a well known  issue with the combination portaudio/alsa under Linux. 
Important: If a name is selected for a channel that cannot be opened the program will try to open the default output device.

Important:
_________

For each of the parameters there is a default.
in the command line will be stored and remembered for a next time.
I.e., if the command

	./linux/dab-cmdline
will assume that the input device is "dabstick", that the Mode is "1",
that the band is "BAND III", that the channel selected is "11C",
that the program we are looking for is "Classic FM", and that the
device to send the output to is "default".

Example
	./linux/dab-cmdline -M 1 -B "BAND III"  -D airspy -C 12C -P "Radio 4" -G 80 -A default
	

For building the dab-cmdline version, a CMakeLists.txt file is available.
A number of settings, in particular for input devices, can be set.

#	set(SDRPLAY true)
#	set(AIRSPY true)
#	set(DABSTICK true)

by default no device is selected, uncomment the lines for the devices
to be included in the configuration.
Obviously, including a device requires the installation of the library
for that device.

#	add_definitions (-DMOT_BASICS)

Uncommenting this line implies that some support for
PAD's (Programme Associated Data) is switched on.


Libraries (together with the "development" or ".h" files) required are:

portaudio, version 19.xxx rather than 18.xxx that is
installed default on some debian based systems;

libsndfile and  libsamplerate

libfaad

libfftw3f

libusb-1.0

zlib

and - obviously, the libraries for the devices included.

If all libraries are in place, build an executable by

mkdir build
cd build
cmake ..
make
make install

========================================================================
The software is made available under the GPL V2.0.

