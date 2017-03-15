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
bufSize		= 10 * 2 * 48000
dataBuffer	= numpy. zeros ((1, bufSize), dtype=float);
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
# callblockHandler is called whenever the library decided
# that an ensemble is there or could not be found
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
    amount = int (amount / 2)
#if the incoming buffer fits, we keep it simple
    if (inp + 2 * amount < bufSize):
       dataBuffer [0, inp : inp + 2 * amount] = buffer;
    else:
#we need to split this up into two faster array assignments
       for x in range (0, 2 * amount - 1):
          dataBuffer [0, (inp + x) % bufSize] = buffer [x];
#and of course we update the inp pointer
    inp = (inp + 2 *  amount) % bufSize;
    
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

    if (avail < 2 * frames):
       amount = int (avail / 2);
    else:
       amount = frames;

#we cannot just do an array assignment, since the outdata is not
#interleaved as the incoming data
    for x in range (0, amount - 1):
       outdata [x, 0] = dataBuffer [0, (outp + 2 * x) % bufSize];
       outdata [x, 1] = dataBuffer [0, (outp + 2 * x + 1) % bufSize];

# if we did not have enough data in the buffer, adjust with zeros
    if (amount < frames):
       for x in range (amount, frames):
          outdata [x, 0] = 0;
          outdata [x, 1] = 0;
# and at last, increment the outp pointer in the data buffer
    outp = (outp + 2 * amount) % bufSize;

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
