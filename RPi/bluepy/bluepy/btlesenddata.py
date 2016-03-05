import btle
import time
import datetime
import MySQLdb
import pdb
from subprocess import Popen, PIPE
import os
import signal
import RPi.GPIO as GPIO
from time import sleep
GPIO.setmode(GPIO.BOARD)

GPIO.setup(40, GPIO.OUT)

LOG_FILE = "IU_log.txt"

def scanForIUDevices():
    try:
        os.system("sudo hciconfig hci0 down; sudo hciconfig hci0 up")
    except:
        print("Could not configure hcitool")
        f = open(LOG_FILE, "a")
        f.write(str(datetime.datetime.now()) + "  Could not configure hcitool" + '\n')
        f.close()
        raise
    try:
        os.system("hcitool lescan > IU_config2.txt & sleep 5; pkill --signal SIGINT hcitool")
    except:
        print("Could not write scanned addresses to config file")
        f = open(LOG_FILE, "a")
        f.write(str(datetime.datetime.now()) + "  Could not write scanned addresses" + '\n')
	f.close()
	raise
    os.system("sudo close(IU_config2.txt)")

    contents=[]
    try:
        with open('IU_config2.txt','r') as f:
            for line in f:
	        contents.append(line)
    except:
        print("Could not open config file")
        f = open(LOG_FILE, "a")
        f.write(str(datetime.datetime.now()) + "  Could not open config file" + '\n')
        f.close()
        raise
    contents=map(str.strip,contents)

    address=[]
    for line in contents:
        if line.find('HMSoft') >= 0:
            templine=line.split('HMSoft')
	    templine[0]=templine[0].strip()
    	    address.append(templine[0])
    
    p=[]
    i=0
    if len(address)<=0:
        print("Could not find any IU devices")
        raise
    for addr in address:
        success = False
        while not success:
            try:
                peri = btle.Peripheral(addr)
                success = True
            except:
                print("Failed; retrying")
                pass
        p.append(peri)
        p[i].setDelegate(TeensyDelegate())
        i+=1
        print(addr)
        
    print "Initializing and Connecting..."
    return p

class TeensyDelegate(btle.DefaultDelegate):
    def __init__(self):
	self.buffer = ""
        btle.DefaultDelegate.__init__(self)

devices = scanForIUDevices()
time_then = time.time()

while True:
    	sleep(1)
	devices[0]._writeCmd("0000-0020-0150-1500")

