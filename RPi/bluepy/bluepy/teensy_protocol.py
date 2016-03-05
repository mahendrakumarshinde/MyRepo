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
#GPIO.output(40, True)
#sleep(1)
#GPIO.output(40, False)
#sleep(1)
#GPIO.output(40, True)
#sleep(1)
#GPIO.output(40, False)
#sleep(1)

#pdb.set_trace()
LOG_FILE = "IU_log.txt"
SENSOR_OUTPUT = "sensor-data.txt"
conn = MySQLdb.connect(host="gator4189.hostgator.com", user="ycchen", passwd="g6n,qMi4aocO", db="ycchen_productivity_database")
x =conn.cursor()
#global conn
#global x
table=None


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
        os.system("hcitool lescan > IU_config.txt & sleep 5; pkill --signal SIGINT hcitool")
    except:
        print("Could not write scanned addresses to config file")
        f = open(LOG_FILE, "a")
        f.write(str(datetime.datetime.now()) + "  Could not write scanned addresses" + '\n')
	f.close()
	raise
    os.system("sudo close(IU_config.txt)")

    contents=[]
    try:
        with open('IU_config.txt','r') as f:
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
    
    def handleNotification(self, cHandle, data):
	self.buffer = self.buffer + data
	if (";" in self.buffer):
	    two = self.buffer.split(";");
	    self.buffer = two[1]
	    msg = two[0]
            msg= msg.replace("\r\n","")
            msg = msg.split(",")
	    print(msg)
	    d8 = str(datetime.datetime.now())
	    f = open(SENSOR_OUTPUT, "a")
	    f.write("[" + d8 + "] " + ' '.join(msg) + '\n')
	    f.close()
	    
	    try:
	        addsqldata="""INSERT INTO IU_device_data (`Timestamp_Pi`, `MAC_ADDRESS`, `State`, `Battery_Level`, `Feature_Value_1`) VALUES (%s, %s, %s, %s, %s)"""
	        x.execute(addsqldata,(d8,msg[0],msg[1],msg[2],msg[4]))
                #thresholds="""SELECT * FROM Feature_dictionary"""
                #x.execute(thresholds)
                #table = x.fetchall()
                
	    except:
		print("Could not access SQL database - check internet connectivity")
		print("Retrying...")
                f = open(LOG_FILE, "a")
                f.write(str(datetime.datetime.now()) + "  SQL connection failed, retrying" + '\n')
                f.close()
		try:
                    global conn
		    global x
                    conn = MySQLdb.connect(host="gator4189.hostgator.com", user="ycchen", passwd="g6n,qMi4aocO", db="ycchen_productivity_database")
                    x = conn.cursor()
                except:
                    print("Reconnect failed.")
                    f = open(LOG_FILE, "a")
                    f.write(str(datetime.datetime.now()) + "  SQL connection failed permanently." + '\n')
                    f.close()
                    GPIO.output(40, False)
                    raise
	    conn.commit()

def update_thresholds(devices):
    thresholds="""SELECT * FROM Feature_dictionary"""
    global x
    x.execute(thresholds)
    tbl=x.fetchall()
    global table
    for row in tbl:
        if table is None or not contains(table, row):
            device = [y for y in devices if y.deviceAddr == row[0]]
            if device:
                device=device[0]
                send_threshold(device, row)
                print("Threshold updated: " + str(row))
            else:
                print("Threshold update failed: device not found")
    table = tbl

def send_threshold(device, threshold):
    characs = device.getCharacteristics()
    for charac in characs:
        print(charac.propertiesToString())
        if (len(charac.propertiesToString().split()) > 3):
            data = "%04d"%threshold[1] + "-%04d"%threshold[3] + "-%04d"%threshold[4] + "-%04d"%threshold[5]
            print(data)
            charac.write(data)

def contains(table, row):
    for tup in table:
        found = True
        if not len(tup) == len(row):
            found = False
        for i in range(len(tup)):
            if not tup[i] == row[i]:
                found = False
        if found:
            return True
    return False

devices = scanForIUDevices()
time_then = time.time()
update_thresholds(devices)
try:
    while True:
        for i in range(len(devices)):
            if devices[i].waitForNotifications(1):
                print("Received data " + str(i+1))
            #pdb.set_trace()
        print("Waiting")
        #pdb.set_trace()
        update_thresholds(devices)
     
except:
    print("Data collection stopped.")
    f = open(LOG_FILE, "a")
    f.write(str(datetime.datetime.now()) + "  Data collection stopped." + '\n')
    f.close()
    GPIO.output(40, False)