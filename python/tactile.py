#!/usr/bin/env python
"""
author: jgoldberg

"""
from __future__ import division
from lib import command
import time,sys,os
import threading
import serial
import shared
import numpy as np

  
import termios
import fcntl

from hall_helpers import *


ROWS = 0
COLS = 0
count = 0
grid = [0,0,0,0]
bpack = 0
packetnumber = 0
mins = np.zeros(0)
maxes = np.zeros(0)
averageFrame = None
averageCount = 0
averageLines = 0
#keyboard polling block
shared.enter = threading.Event()
class KeyboardPoller( threading.Thread ) :
    def run( self ) :
        #global key_pressed
        #ch = sys.stdin.read( 1 )
        while True:
            #ch = myGetch()
            #print "pressed:",ch
            raw_input()
            #print "enter"
            shared.enter.set()
        #if ch == 'K' : # the key you are interested in
        #    key_pressed = 1
        #else :
        #    key_pressed = 0

def myGetch():
    fd = sys.stdin.fileno()

    oldterm = termios.tcgetattr(fd)
    newattr = termios.tcgetattr(fd)
    newattr[3] = newattr[3] & ~termios.ICANON & ~termios.ECHO
    termios.tcsetattr(fd, termios.TCSANOW, newattr)

    oldflags = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, oldflags | os.O_NONBLOCK)

    try:        
        while 1:            
            try:
                c = sys.stdin.read(1)
                #break
                return c
            except IOError: pass
    finally:
        termios.tcsetattr(fd, termios.TCSAFLUSH, oldterm)
        fcntl.fcntl(fd, fcntl.F_SETFL, oldflags)
#end keyboard polling block

def main():
    global poller
    poller = KeyboardPoller()
    poller.start()

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
    #samplePixel(row, col)
    #sampleFrame(period)
    #time.sleep(3)
    #sampleFrame(period)
    #time.sleep(3)
    #sampleFrame(period)
    #time.sleep(3)
    #pollPixel(row, col, dur, period)
    #for i in range(1000):
    global bpack
    while True:
        #print "sending test frame", bpack
        bpack = bpack + 1
        #testFrame()
        sampleFrame(period)
        time.sleep(.02)
        #queryRobot()
        '''
        samplePixel(1, 4)
        time.sleep(.05)
        samplePixel(2, 4)
        time.sleep(.05)
        samplePixel(6, 4)
        time.sleep(.05)
        samplePixel(11, 4)
        time.sleep(.05)
        '''
        #print "main",enter.isSet()
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
    global count
    global grid
    global packetnumber
    global mins
    global maxes
    global averageFrame
    global averageCount
    global averageLines
    #print "----------"
    #print "mode:", data[0]
    #for i in range(0,len(data)):
    #    print "data: ", ord(data[i])
    if data[0] == 'A' or data[0] == 'C':
        #print "row:", ord(data[1]), "col:", ord(data[2])
        val = ord(data[3]) + (ord(data[4])<<8)
        #print "value =", val
        grid[count] = val
        count = count + 1
        if count == 4:
            count = 0
            packetnumber = packetnumber + 1
            print packetnumber
            print grid[2], grid[1]
            print grid[3], grid[0]
        
    elif data[0] == 'B':
        print "received B packet", bpack
        bpack = bpack + 1
        #bpack = bpack + 1
        if ROWS == 0 or COLS == 0:
            print "ERROR: Row and column size hasn't been set"
            return
        #print len(data)
        temp = np.zeros(len(data))
        for i in range(len(data)):
            temp[i] = ord(data[i])
        temp = np.uint8(temp)
        #print list(data)
        #print temp
        frame = temp[1:-1:2] + (temp[2::2]*256)
        #print frame
        # this does calibration
        newframe = np.zeros(ROWS*COLS)
        for i in range(ROWS*COLS):
            if frame[i] < mins[i]:
                mins[i] = frame[i]
            elif frame[i] > maxes[i]:
                maxes[i] = frame[i]
            newframe[i] = (frame[i] - mins[i]) / (maxes[i] - mins[i])
        #newframe = np.reshape(newframe, (ROWS,COLS))
        #print " ", newframe[3,0], newframe[4,0]
        #print newframe[2,0], "   ", newframe[5,0]
        #print " ", newframe[1,0], newframe[0,0]
        
        #newframe = newframe * 100
        print mins
        print maxes

        print("    %4.f  " % (frame[5]))
        print("%4.f    %4.f" % (frame[4], frame[0]))
        print("%4.f    %4.f" % (frame[3], frame[1]))
        print("    %4.f  " % (frame[2]))

        print("    %.2f  " % (newframe[5]))
        print("%.2f    %.2f" % (newframe[4], newframe[0]))
        print("%.2f    %.2f" % (newframe[3], newframe[1]))
        print("    %.2f  " % (newframe[2]))

        averageMax = 50.0
        if shared.enter.isSet():
            if averageCount == 0:
                averageFrame = newframe
            elif averageCount < averageMax:
                averageFrame = averageFrame + newframe
            averageCount = averageCount + 1
            if averageCount == averageMax:
                averageFrame = averageFrame / averageMax
                print "#############"
                print averageFrame
                print "#############"
                fd = open('test.csv','a')
                if averageLines == 0:
                    myCsvRow = "Normalized Averages,Samples = " + str(averageMax) + "\n"
                    fd.write(myCsvRow)
                myCsvRow = str(averageLines) + ","
                for i in range(len(averageFrame)):
                    myCsvRow = myCsvRow + "," + str(averageFrame[i])
                myCsvRow = myCsvRow + "\n"
                fd.write(myCsvRow)
                fd.close()
                averageCount = 0
                averageLines = averageLines + 1
                shared.enter.clear()
            
            #print "set"

        #print("  %.2f  %.2f" % (newframe[3], newframe[4]))
        #print("%.2f      %.2f" % (newframe[2], newframe[5]))
        #print("  %.2f  %.2f" % (newframe[1], newframe[0]))
    elif data[0] == 'G':
        ROWS = ord(data[1])
        COLS = ord(data[2])
        print "shell has", ROWS, "rows and", COLS, "columns."
        
        #calibrated values hardcoded
        mins = np.ones(ROWS*COLS) * 0
        mins[2] = 121
        maxes = np.ones(ROWS*COLS) * 4024
        maxes[1] = 4025

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
        poller._Thread__stop()
    #except Exception as args:
    #    print "\nGeneral exception:",args
    #    print "Attemping to exit cleanly..."
    #    shared.xb.halt()
    #    shared.ser.close()
    #    sys.exit()
    #except serial.serialutil.SerialException:
    #    shared.xb.halt()
    #    shared.ser.close()
