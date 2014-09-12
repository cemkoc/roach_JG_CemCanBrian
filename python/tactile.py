#!/usr/bin/env python
"""
author: jgoldberg

"""
from lib import command
import time,sys,os
import serial
import shared
import numpy as np

from hall_helpers import *

ROWS = 0
COLS = 0

bpack = 1

def main():
    
    setupSerial()
    #return
    # Send robot a WHO_AM_I command, verify communications
    queryRobot()
    time.sleep(1)
    
    getSkinSize()
    time.sleep(2)
    row = 0
    col = 0
    dur = 15
    period = 500
    samplePixel(row, col)
    #sampleFrame(period)
    #time.sleep(3)
    #sampleFrame(period)
    #time.sleep(3)
    #sampleFrame(period)
    #time.sleep(3)
    #pollPixel(row, col, dur, period)
    for i in range(1000):
        #print "sending test frame", i
        #testFrame()
        #queryRobot()
        samplePixel(row, col)
        time.sleep(.5)
    
    period = 250
    #while True:
    #    sampleFrame(period)
    #    print "sent sample command"
    #    time.sleep(1)


def samplePixel(row, col):
	xb_send(0, command.TACTILE, 'A' + chr(row) + chr(col))

def sampleFrame(period):
    #period in microseconds
    xb_send(0, command.TACTILE, 'B' + chr(period % 256) + chr(period >> 8))

def pollPixel(row, col, duration, period):
    #duration in seconds
    #period in milliseconds (must be < 256)
    xb_send(0, command.TACTILE, 'C' + chr(row) + chr(col) + chr(duration) + chr(period))
    time.sleep(duration + 2)

def getSkinSize():
    xb_send(0, command.TACTILE, 'G')


def testFrame():
    xb_send(0, command.TACTILE, 'T')

previous = -1
skip = 0
def handleTactilePacket(data):
    global ROWS
    global COLS
    global bpack
    global previous
    global skip
    #print "----------"
    #print "mode:", data[0]
    #for i in range(0,len(data)):
    #    print "data: ", ord(data[i])
    if data[0] == 'A' or data[0] == 'C':
        #print "row:", ord(data[1]), "col:", ord(data[2])
        print "value =", ord(data[3]) + (ord(data[4])<<8)
    elif data[0] == 'B':
        print "received B packet", bpack
        bpack = bpack + 1
        if ROWS == 0 or COLS == 0:
            print "ERROR: Row and column size hasn't been set"
            return
        print len(data)
        temp = np.zeros(len(data))
        for i in range(0,len(data)):
            temp[i] = ord(data[i])
        temp = np.uint8(temp)
        #print list(data)
        print temp
        '''frame = temp[1:-1:2] + (temp[2::2]*256)
        frame = np.reshape(frame, (ROWS,COLS))
        print frame'''
    elif data[0] == 'G':
        ROWS = ord(data[1])
        COLS = ord(data[2])
        print "shell has", ROWS, "rows and", COLS, "columns."

    elif data[0] == 'T':
        '''for i in range(len(data)):
            if i == 0:
                print data[i]
            else:
                print ord(data[i])'''
        print map(ord, data)
    elif data[0] == 'X':
        print ord(data[0]),ord(data[-1])
        if ord(data[0]) != (previous + 1) % 256:
            skip = skip + 1 
            print "skip:",skip/float(previous)
            
        previous = ord(data[0])

#Provide a try-except over the whole main function
# for clean exit. The Xbee module should have better
# provisions for handling a clean exit, but it doesn't.
if __name__ == '__main__':
    try:
        main()
        #time.sleep(6)
        while True:
            time.sleep(1)
        print "----------"
        #xb_safe_exit()
    except KeyboardInterrupt:
        print "\nRecieved Ctrl+C, exiting."
        shared.xb.halt()
        shared.ser.close()
    #except Exception as args:
    #    print "\nGeneral exception:",args
    #    print "Attemping to exit cleanly..."
    #    shared.xb.halt()
    #    shared.ser.close()
    #    sys.exit()
    #except serial.serialutil.SerialException:
    #    shared.xb.halt()
    #    shared.ser.close()
