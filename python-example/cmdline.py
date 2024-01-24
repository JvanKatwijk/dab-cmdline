#
#    Copyright (C) 2017
#    Jan van Katwijk (J.vanKatwijk@gmail.com)
#    Lazy Chair Computing
#
#    This file is the python wrapper for the DAB-library.
#    DAB-library is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    DAB-library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with DAB-library, if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

import argparse
import time

import numpy
import sounddevice as sd
from libdab_lib import *

# example program for use with "libdab_lib.so"
# it is recommended to read the README before running
# default values
theDevice = None
theLibrary = None
theBand = 0
serviceId = -1  # not used here
run = 0  # global flag

# the simulated queue for passing data from the callback to the sounddevice
# is large, it may contain to up to 10 seconds of sound
bufSize = 10 * 48000
dataBuffer = numpy.zeros((bufSize, 2), dtype=numpy.float32)
inp = 0
outp = 0
#
#
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('-d', '--theDelay', type=int, default=15,
                    help=' a number to specify the time to find an ensemble')
parser.add_argument('-m', '--theMode', type=int, default=1,
                    help='1, 2 or 4 for the DAB mode')
parser.add_argument('-g', '--theGain', type=int, default=50,
                    help='relative gain for input device')
parser.add_argument('-a', '--soundChannel', type=str, default='default',
                    help='the sound channel')
parser.add_argument('-c', '--theChannel', type=str, default="11C",
                    help='the Channel')
parser.add_argument('-p', '--programName', type=str, default="538",
                    help='name of the program')
parser.add_argument('-l', '--latency', type=float, default=0.1,
                    help='latency in seconds')
args = parser.parse_args()

sd.default.device = args.soundChannel

#
# The callbacks for which there is support are given here, most
# of them with an empty implementation. Feel free to adapt
#
ensembleFound = 0  # until shown otherwise
signalSet = 0
noData = 0


# as soon as reasonable, the underlying implementation issues
# a signal to tell whether or not there might be DAB data.
# if no DAB data is found, it is almost certain that there is
# no DAB data. If the signal indicates that DAB data might have
# been found, some form of synchronization is reached, but it
# does not mean that DAB signals can be decoded
def syncsignalHandler(b):
    global signalSet
    global noData
    if signalSet:
        return
    if b:
        print("possible DAB signal detected")
        noData = 0
        signalSet = 1
    else:
        noData = noData + 1
        if noData > 40:
            print("no DAB signal detected, giving up")
            signalSet = 1


# the function systemdatahandler is called (once a second) to show
# whether we are in sync, the snr and the current frequency offset
def systemdataHandler(b, snr, offs):
    # print("systemdataHandler b=", b, " snr=", snr, " offs=", offs)
    pass

# the function ensemblenameHandler is called as soon as the name
# of the ensemble is extracted from the datastream. The second
# parameter is the ID of the ensemble
def ensemblenameHandler(s, d):
    global ensembleFound
    print("name of ensemble is ", s, " ", d)
    ensembleFound = 1


# the function programnameHandler is called for each program that is
# found in the datastream, the first parameter is the name, the
# second the ID
def programnameHandler(p, d):
    print("program ", p, " ", d, " is part of the ensemble")


# the function fib_qualityHandler is called regularly, its parameter
# shows the percentage of ficBlocks that passed the CRC test
def fib_qualityHandler(q):
    if q != 100:
        print(" fib ", q, "\r")


# the function dataoutHandler is called whenever the inputstream contains
# data to be displayed as label
def dataoutHandler(d):
    print("dataoutHandler = ", d)


# the function motDataHandler is called whenever the inputstream contains
# data to be displayed as picture
def motDataHandler(data, name, d):
    print("motDataHandler: %s of size %d" % (name, len(data)))


def programdataHandler(q0, q1, q2, q3, q4):
    print("selected program ")
    print("   subchannel   ", q0)
    print("   start address", q1)
    print("   length       ", q2)
    print("   bitRate      ", q3)
    if q4 == 63:
        print("   DAB+")
    else:
        print("   DAB")


# the function program_qualityHandler is called on a regular basis
# and shows the result of the various checks that are done on the
# DAB and DAB+ data (see the dab-api.h)
def program_qualityHandler(q0, q1, q2):
    print("qualityHandler q0=", q0, " q1=", q1, " q2=", q2)


# audioOut is called whever there is sound.
def audioOutHandler(buffer, size, rate):
    global dataBuffer
    global inp, bufSize
    # we first determine whether there is space in the queue
    if outp >= inp:
        space = outp - inp
    else:
        space = bufSize - inp + outp
    if size > space:
        amount = space
    else:
        amount = size
    # and just skip incoming samples if the queue is (almost) full
    # if the incoming buffer fits, we keep it simple
    if inp + amount < bufSize:
        dataBuffer[inp: inp + amount, 0] = buffer[:, 0]
        dataBuffer[inp: inp + amount, 1] = buffer[:, 1]
    else:
        # we need to split this up into two faster array assignments
        for x in range(0, amount):
            dataBuffer[(inp + x) % bufSize, 0] = buffer[x, 0]
            dataBuffer[(inp + x) % bufSize, 1] = buffer[x, 1]
    # and of course we update the inp pointer
    inp = (inp + amount) % bufSize


# sound_callback is called by the sound software whenever that is ready to
# handle a new buffer (the client better be prepared to have data available)
def sound_callback(outdata, frames, time, status):
    global dataBuffer, outp, bufSize
    #
    # we first determine whether the requested amount is available
    if inp > outp:
        avail = inp - outp - 1
    else:
        avail = bufSize - outp + inp - 1

    if avail < frames:
        amount = avail
    else:
        amount = frames
    #
    # we then copy the data from the queue to the array that was passed
    # as parameter.

    for x in range(0, amount):
        outdata[x, 0] = dataBuffer[(outp + x) % bufSize, 0]
        outdata[x, 1] = dataBuffer[(outp + x) % bufSize, 1]

    # if we did not have enough data in the buffer, adjust with zeros
    if (amount < frames):
        for x in range(amount, frames):
            outdata[x, 0] = 0
            outdata[x, 1] = 0
    # and at last, increment the outp pointer in the data buffer
    outp = (outp + amount) % bufSize


# real work, initialize the library, note that the dab_initialize
# function could have more callback functions with it.
theLibrary = dabInit(args.theChannel,
                     args.theGain,
                     args.theMode,
                     syncsignalHandler,
                     systemdataHandler,
                     ensemblenameHandler,
                     programnameHandler,
                     fib_qualityHandler,
                     audioOutHandler,
                     dataoutHandler,
                     motDataHandler,
                     programdataHandler,
                     program_qualityHandler
                     )

if not dabProcessing(theLibrary):
    print("could not start device, fatal")
    exit(3)

count = args.theDelay
while (count > 0 and not ensembleFound and noData < 5):
    time.sleep(1)
    count = count - 1

if (not ensembleFound):
    print("no ensemble found, giving up")
    exit(5)

time.sleep(2)
dabService(args.programName, theLibrary)
with sd.OutputStream(samplerate=48000,
                     channels=2,
                     dtype='float32',
                     blocksize=1024,
                     latency=args.latency,
                     callback=sound_callback):
    print("#" * 80)
    print("press Return to quit")
    print("#" * 80)
    input()

dabStop(theLibrary)
dabExit(theLibrary)
