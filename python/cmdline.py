import numpy
import time
import threading
from collections import deque
import sounddevice as sd
import argparse
from dablib import *

import sys

#default values
theRadio	= None
theBand		= 0
serviceId	= -1	# not used here
run		= 0	# global flag

#the queue for passing data from the callback to the sounddevice
q		= deque ()	# intermediate buffer

#
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('-d', '--theDelay', type=int, default=10,
                    help=' a number to specify the time to find an ensemble')
parser.add_argument('-m', '--theMode', type=int, default=1,
                    help='1, 2 or 4 for the DAB mode')
parser.add_argument('-g', '--theGain', type=int, default=80,
                                        help='relative gain for input device')
parser.add_argument('-a', '--soundChannel', type=str, default='default',
	                                help='the sound channel')
parser.add_argument('-c', '--theChannel', type=str, default = "11C",
	                                help='the Channel')
parser.add_argument('-p', '--programName', type=str, default="Classic FM",
	                                help='name of the program')
parser.add_argument('-l', '--latency', type=float, default = 0.1,
	                                help='latency in seconds')
args = parser.parse_args()

sd.default.device = args. soundChannel

# a really bad name that has to be changed
def	dummy (a, b, c, d, e):
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
       dab_Service (theRadio, args. programName, dummy);
    else:
       run = 0;
       print ("no ensemble found");
       exit (2);

# labelhandler is called whenever there is text for the dynamic label
def labelHandler (s):
#    print (s)
    a = s

# pcmHandler is called whever there is sound.
def pcmHandler (buffer, size, rate):
    global q
    for x in range (0, size - 1):
       q. append (float (buffer [x]));

# callback is called by the sound software whenever that is ready to 
# put a new buffer into the sound system
def callback (outdata, frames, time, status):
    global q
    l = len (q);
    if (l <= 2 * frames):
       for x in range (0, int (l / 2) - 1):
          outdata [x, 0] = q. popleft ();
          outdata [x, 1] = q. popleft ();
       for x in range (l, frames - 1):
          outdata [x, 0] = 0;
          outdata [x, 1] = 0;
    else:
       for x in range (0, frames - 1):
          outdata [x, 0] = q. popleft ();
          outdata [x, 1] = q. popleft ();

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
                       blocksize  = 256,
	               latency    = args. latency,
	               callback   = callback):
   while (run):
      time. sleep (10);

dab_stop (theRadio);
dab_exit (theRadio);
