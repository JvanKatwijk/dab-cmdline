import numpy
import time
import sounddevice as sd
import argparse
from dablib import *
import sys

#default values
theRadio	= None
theBand		= 0
serviceId	= -1	# not used here
run		= 0	# global flag

#the simulated queue for passing data from the callback to the sounddevice
#is large, it may contain to up to 10 seconds of sound
bufSize		= 10 * 48000
dataBuffer	= numpy. zeros ((bufSize, 2), dtype=float);
inp		= 0;
outp		= 0;
#
parser		= argparse. ArgumentParser (description=__doc__)
parser.add_argument ('-d', '--theDelay', type=int, default=10,
                     help=' a number to specify the time to find an ensemble')
parser.add_argument ('-m', '--theMode', type=int, default=1,
                     help='1, 2 or 4 for the DAB mode')
parser.add_argument ('-g', '--theGain', type=int, default=80,
                                        help='relative gain for input device')
parser.add_argument ('-a', '--soundChannel', type=str, default='default',
	                                help='the sound channel')
parser.add_argument ('-c', '--theChannel', type=str, default = "11C",
	                                help='the Channel')
parser.add_argument ('-p', '--programName', type=str, default="Classic FM",
	                                help='name of the program')
parser.add_argument ('-l', '--latency', type=float, default = 0.1,
	                                help='latency in seconds')
args		= parser.parse_args ()

sd.default.device = args. soundChannel

def	program_data (a, b, c, d, e):
   print ("startaddress = ", a);
   print ("length       = ", b);
   print ("subchId      = ", c);
   print ("protection   = ", d);
   print ("bitrate      = ", e);

#
# callblockHandler is called when the library has decided
# that either an ensemble is there or none could be found
#The thing to be done is then to select a program
def	callblockHandler (s, b):
    global run
    if (b):
       print (s)
       print ("selected program is " + args. programName)
       dab_Service (theRadio, args. programName, program_data);
    else:
       run = 0;
       print ("no ensemble found, giving up");
       exit (2);

# labelhandler is called whenever there is text for the dynamic label
def labelHandler (s):
#    print (s)
    a = s

# pcmHandler is called whever there is sound.
def pcmHandler (buffer, size, rate):
    global dataBuffer;
    global inp, bufSize;
# we first determine whether there is space in the queue
    if (outp >= inp):
       space = outp - inp;
    else:
       space = bufSize - inp + outp;
    if (size > space):
       amount = space;
    else:
       amount = size;
#and just skip incoming samples if the queue is (almost) full
#if the incoming buffer fits, we keep it simple
    if (inp + amount < bufSize):
       dataBuffer [inp : inp + amount, 0] = buffer [:,0];
       dataBuffer [inp : inp + amount, 1] = buffer [:,1];
    else:
#we need to split this up into two faster array assignments
       for x in range (0, amount - 1):
          dataBuffer [(inp + x) % bufSize, 0] = buffer [x, 0];
          dataBuffer [(inp + x) % bufSize, 1] = buffer [x, 1];
#and of course we update the inp pointer
    inp = (inp + amount) % bufSize;

# callback is called by the sound software whenever that is ready to 
# handle a new buffer (the client better be prepared to have data available)
def sound_callback (outdata, frames, time, status):
    global dataBuffer, outp, bufSize;
#
#we first determine whether the requested amount is available
    if (inp > outp):
       avail = inp - outp - 1;
    else:
       avail = bufSize - outp + inp - 1;

    if (avail < frames):
       amount = avail;
    else:
       amount = frames;
#
# we then copy the data from the queue to the array that was passed
# as parameter.

    for x in range (0, amount - 1):
       outdata [x, 0] = dataBuffer [(outp + x) % bufSize, 0];
       outdata [x, 1] = dataBuffer [(outp + x) % bufSize, 1];

# if we did not have enough data in the buffer, adjust with zeros
    if (amount < frames):
       for x in range (amount, frames):
          outdata [x, 0] = 0;
          outdata [x, 1] = 0;
# and at last, increment the outp pointer in the data buffer
    outp = (outp + amount) % bufSize;

#
# real work, initialize the library
theRadio	= dab_initialize (args. theMode,
	                          theBand,
	                          pcmHandler,
	                          labelHandler,
	                          args. theDelay);
if (theRadio == None):
    print ("sorry, no radio available, fatal\n");
    exit (4);

dab_Gain	(theRadio, args. theGain);
dab_channel	(theRadio, args. theChannel);
dab_run		(theRadio, callblockHandler);
run	= 1
with sd. OutputStream (samplerate = 48000,
	               channels   = 2,
	               dtype      = 'float32',
                       blocksize  = 1024,
	               latency    = args. latency,
	               callback   = sound_callback):
   while (run):
      time. sleep (10);

dab_stop (theRadio);
dab_exit (theRadio);
