

dab-scanner
----------------------------------------------------------------------
---------------------------------------------------------------------
disclaimer

	As far as I can see the program does what I think it should do.
	The program - and the sources - are available. If there is something
	you do not like or want to have changed: Feel free to make changes,
	no need to bother me.
-------------------------------------------------------------------------
-------------------------------------------------------------------------

scanner is a completely rewritten (simple) dab-scanner, using 
software components from the latest (6.9.4) version of Qt-DAB.

The scanner will perform a SINGLE scan over the DAB channels in the old
Band III region (i.e. app 175 .. 230 Mhz) and store the results
automatically in a file.

Output is
a. on the terminal for each channel where DAB signals were detected
a brief description of the ensemble and a list of services as shown below

	channel 5B -> EId 805B	ensembleName 5B Z-H/Zeeland  
	840D	KISS            
	8AB1	ARROW BLUESBOX  
	841B	Omroep Zeeland  
	841A	Radio Rijnmond  
	845A	Amor FM         
	8421	RADIONL         
	8419	Omroep West     
	8E02	HofStad Radio   
	8998	Grand Prix Radio
	8DAF	Sunrise FM      
	8DAD	GLXY.RADIO      
	84AE	THE GROOVE   

b. a Json file is created with more details on the ensemble and the
services.


----------------------------------------------------------------------------
Building an executable
----------------------------------------------------------------------------

an executable is built agains one input device. The supported input devices
are listen in the CMakeLists.txt files.

Assuming you want to build with an SDRplay device

	mkdir build
	cd build
	cmake .. -DSDRPLAY_V3=ON
	make

should do it.

required libraries are fftw3f, zlib and pthreads

--------------------------------------------------------------------------
Running the program
--------------------------------------------------------------------------

If building the program is successful, it is named (with the setting above)

	dab-scanner-sdrplay-v3

and it still resides in the "build" directory

When running the program , it assumed you have installed the
so-called tii database. For x64 Linux, a small utility exists
(see Qt-DAB), anyone can load a database from the repository of Qt-DAB

Command line parameters are in two categories

a. a few for setting the decoding process,
b. a few to set the parameters for the device

Parameters for the decoding process set basically **waiting times**

	the waiting time to see whether or not a DAB signal could be
	identified is fixed;

	one a form of time syncing could be done, the software will wait
	for default 8 seconds to see whether or not an ensemblename
	couold be detected.
	The value can be set by the -D parameter

	once an ensemble - and hopefully some services - were identified
	the software will wait for another (default) 10 seconds to
	collect TII data (if any).
	The value can be set by the -I parameter

	The output is written to a file. If no filename is specified,
	it is written in "test.json" in the directory in which the
	program is instatiated.
	The -F parameter allows specifying a filename

Parameters for device settings are essentially those for gain settings.

	
