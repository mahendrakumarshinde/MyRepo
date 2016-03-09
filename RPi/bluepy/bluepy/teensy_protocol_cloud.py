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

"""
GPIO SETUP (Green LED)
"""

GPIO.setmode(GPIO.BOARD)
GPIO.setwarnings(False)

GPIO.setup(40, GPIO.OUT)
GPIO.output(40, True)
sleep(1)
GPIO.output(40, False)
sleep(1)
GPIO.output(40, True)
sleep(1)
GPIO.output(40, False)
sleep(1)

"""
STARTUP CODE
"""

LOG_FILE = "IU_log.txt"
SENSOR_OUTPUT = "sensor-data.txt"
try:
    conn = MySQLdb.connect(host="173.194.252.192", user="infinite-uptime", passwd="infiniteuptime", db="Hub_Dictionary")
    x =conn.cursor()
except:
    report_error("Unable to access hub dictionary.")
    raise
table=None
time_start = time.time()

# Retrieves the serial number of this Raspberry Pi.
def getserial():
    cpuserial = "0000000000000000"
    try:
        f = open('/proc/cpuinfo', 'r')
        for line in f:
            if line[0:6]=='Serial':
                cpuserial = line[10:26]
        f.close()
    except:
        cpuserial="ERROR00000000000"
        report_error("RPi serial number could not be found.")
        raise
    return cpuserial

# Scans for nearby Infinite Uptime devices and connects to those labeled "HMSoft".
# Generates Peripheral objects and sets their delegates to TeensyDelegate instances.
def scanForIUDevices():
    try:
        os.system("sudo hciconfig hci0 down; sudo hciconfig hci0 up")
    except:
        report_error("Could not configure hcitool")
        raise
    try:
        os.system("hcitool lescan > IU_config.txt & sleep 10; pkill --signal SIGINT hcitool")
    except:
        report_error("Could not write scanned addresses to config file")
	raise
    os.system("sudo close(IU_config.txt)")

    contents=[]
    try:
        with open('IU_config.txt','r') as f:
            for line in f:
	        contents.append(line)
    except:
        report_error("Could not open config file")
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
	print("Rescanning")
	GPIO.output(40, False)
        p = scanForIUDevices()
	return p
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

# Subclass from btle.py. handleNotification processes incoming BLE messasges, 20 bytes at a time.
# Parses the data, and when a complete message is found, uploads data to the SQL database.
class TeensyDelegate(btle.DefaultDelegate):
    def __init__(self):
	self.buffer = ""
        btle.DefaultDelegate.__init__(self)
    
    def handleNotification(self, cHandle, data):
	print("Start handleNotification: " + str(time.time() - time_start))
        self.buffer = self.buffer + data
        if (";" in self.buffer):
	    two = self.buffer.split(";");
	    self.buffer = two[1]
	    msg = two[0]
            msg= msg.replace("\r\n","")
            msg = msg.split(",")
	    d8 = str(datetime.datetime.now())
	    #msg[0] = '68:9E:19:07:'+msg[0]
	    f = open(SENSOR_OUTPUT, "a")
	    f.write("[" + d8 + "] " + ' '.join(msg) + '\n')
	    f.close()
	    
	    print("handleNotfication: sending to SQL at Time: " + str(time.time() - time_start) + " Message: " + str(msg))
	    try:
	        addsqldata="""INSERT INTO IU_device_data (`Timestamp_Pi`, `MAC_ADDRESS`, `State`, `Battery_Level`, `Feature_Value_0`, `Feature_Value_1`, `Feature_Value_2`, `Feature_Value_3`, `Feature_Value_4`, `Feature_Value_5`) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	        x.execute(addsqldata,(d8,msg[0],msg[1],msg[2],msg[4],msg[6],msg[8],msg[10],msg[12],msg[14]))
         	print('write to database=' + str(time.time()-time_start))
	    except:
		raise
		report_error("Could not access SQL database - check internet connectivity")
		print("Retrying...")
		try:
                    global conn
		    global x
                    conn = MySQLdb.connect(host="173.194.252.192", user="infinite-uptime", passwd="infiniteuptime", db="InfiniteUptime")
                    x = conn.cursor()
                except:
                    report_error("Reconnect failed.")
                    raise
	    conn.commit()
            return 'done'
	print("End handleNotification: " + str(time.time() - time_start))

# Updates the locally kept list of thresholds based on the SQL database.
# Enters any new devices into the Feature Dictionary. 
# Also updates STATUS for all devices based on whether they have been found.
def check_for_thresholds(devices):
    macAddrs = """SELECT DISTINCT MAC_ADDRESS FROM Feature_Dictionary"""
    global x
    global tableStatus
    x.execute(macAddrs)
    macAddrs = x.fetchall()
    if not macAddrs:
        log_message("No devices assigned.")
    else:
        macAddrs = list(list(zip(*macAddrs))[0])
    	
    for device in devices:
	if device.deviceAddr not in macAddrs:
	    print("Set up " + str(device.deviceAddr))
	    newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Status) VALUES (%s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,(device.deviceAddr,"0","Signal Energy", "30","600","1200", "1"))
            newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Status) VALUES (%s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device.deviceAddr,"1","Mean Crossing Rate", "100","500","1000", "1"])
	    newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Status) VALUES (%s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device.deviceAddr,"2","Spectral Centroid", "6","7","8", "1"])
	    newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Status) VALUES (%s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device.deviceAddr,"3","Spectral Flatness", "40","42","45", "1"])
            newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Status) VALUES (%s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device.deviceAddr,"4","Accel Spectral Spread", "200","205","210", "1"])
	    newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Status) VALUES (%s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device.deviceAddr,"5","Audio Spectral Spread", "500","1000","1500", "1"])
    for macaddress in macAddrs:
	if macaddress not in [y.deviceAddr for y in devices]:
	    printStatus = "Disconnected " + str(macaddress)
	    setStatus = """UPDATE Feature_Dictionary SET Status=%s WHERE MAC_ADDRESS=%s"""
	    statusFlag = 0
	else:
	    printStatus = "Connected " + str(macaddress)
	    setStatus = """UPDATE Feature_Dictionary SET Status=%s WHERE MAC_ADDRESS=%s"""
	    statusFlag = 1
	latestStatus = """SELECT Status FROM Feature_Dictionary WHERE MAC_ADDRESS=(%s) AND Feature_ID=(%s)"""
        x.execute(latestStatus,(macaddress,'0'))
	latestStatus=x.fetchall()
        latestStatus = latestStatus[0][0]
        if statusFlag != latestStatus: #change status in database only if status has changed
            x.execute(setStatus,(str(statusFlag), macaddress))
	    print(printStatus)
    conn.commit()   

# Given a list of devices, check if any thresholds have been updated in the database. If so, update using send_threshold().
def update_thresholds(devices):
    thresholds="""SELECT MAC_ADDRESS, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3 FROM Feature_Dictionary"""
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
            else:
                log_message("Threshold update failed: device not found -" + row[0])
    table = tbl

# Given a changed set of thresholds, update the appropriate device.
def send_threshold(device, threshold):
    characs = device.getCharacteristics()
    for charac in characs:
        if (len(charac.propertiesToString().split()) > 3):
            data = "%04d"%threshold[1] + "-%04d"%threshold[3] + "-%04d"%threshold[4] + "-%04d"%threshold[5]
            log_message("Threshold written: " + data)
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

# Report an error and shut off the LED.
def report_error(message):
    print(message)
    f = open(LOG_FILE, "a")
    f.write(str(datetime.datetime.now()) + "  Error: " + message + '\n')
    f.close()
    GPIO.output(40, False)

# Report a message without shutting off the LED.
def log_message(message):
    print(message)
    f = open(LOG_FILE, "a")
    f.write(str(datetime.datetime.now()) + "  " + message + '\n')
    f.close()

"""
================
Main Code
================
"""

# Determine which database this Pi corresponds to.
try:
    serial = getserial()
    x.execute("SELECT Database_Name FROM Hub_Mapping WHERE Hub_ID='%s'" % serial)
    db_name = x.fetchall()[0][0]
except:
    report_error("Could not find database for serial number: " + serial)
    raise

# Switch connection to the appropriate database, if found.
try:
    conn = MySQLdb.connect(host="173.194.252.192", user="infinite-uptime", passwd="infiniteuptime", db=db_name)
    x =conn.cursor()
except:
    report_error("Connection to database failed.")
    raise

# Find Infinite Uptime devices in the area by calling scanForIUDevices, and update their thresholds using check_for_thresholds and update_thresholds.
devices = scanForIUDevices()
check_for_thresholds(devices)
try:
    update_thresholds(devices)
except btle.BTLEException:
    print("Device disappeared, rescanning")
    devices = scanForIUDevices()


# Start data collection loop
try:    
    while True:
	print("Start while: " + str(time.time() - time_start))
	try:
            GPIO.output(40, True)
	    for i in range(len(devices)):
               while devices[i].waitForNotifications(0.001):
                    print("Received data " + str(i+1))
	    reset = """SELECT reset_value FROM Env_variables"""
	    x.execute(reset)
	    reset = x.fetchall()
	    conn.commit()
	    reset = int(list(list(zip(*reset))[0])[0])
	    if len(devices)>0:
		check_for_thresholds(devices)
		update_thresholds(devices)
	except btle.BTLEException:
	    reset = 1
	if reset:
	    print('Syncing...hold on')
	    devices = scanForIUDevices()
	    check_for_thresholds(devices)
	    update_thresholds(devices)
	    resetReset = """UPDATE Env_variables SET reset_value=(%s)"""
	    x.execute(resetReset,['0'])
	    conn.commit()
except:
    report_error("Data collection stopped.")
