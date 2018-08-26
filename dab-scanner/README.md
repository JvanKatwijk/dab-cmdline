

dab-scanner-xxx 

--------------------------------------------------------------------------

--------------------------------------------------------------------------

![dab scanner with sdrplay input](/dab-scanner/dab-scanner.png?raw=true)

dab-scanner-xxx is a simple command line program, based on the DAB library.
It will perform a scan over all channels in the selected band and
- for those channels where DAB signals are identifier - list the
contents in a format compatible with input for excel.

The input device is fixed, depends on the setting in the CMakeLists.txt
file. The output is either to the terminal or written onto a file.
(-F option).

All callbacks are defined, most of them with an empty body.

See the file main.cpp for the command line options

FEEL FREE TO IMPROVE THE PROGRAM

