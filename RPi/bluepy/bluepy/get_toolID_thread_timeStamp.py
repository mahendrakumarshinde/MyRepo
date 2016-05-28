import xml.etree.ElementTree as ET
import urllib2
import sys
import re
import btle
import time
import datetime
from datetime import datetime
import MySQLdb
import pdb
from subprocess import Popen, PIPE
import os
import signal
import RPi.GPIO as GPIO
from time import sleep


toolID_timestamp_reference = {}
toolID_timestamp_reference['7C:EC:79:61:8B:7A'] = [[1, "00:00:12"], [2, "00:00:19.005"], [3, "00:00:21.005"], [4, "00:00:25.005"], [3, "00:00:28.005"], [14, "00:00:31.005"]]
float_toolID_timestamp_reference={}

#============================ HELPER FUNCTION SECTION ==========================================================

# This function returns the current time
def get_currentTime():
    
    current_time = 0
    try:
	current_time = time.time()
        return current_time
    except:
        print("Error: Cannot get current time.")
        return 1


# This function returns True if the machine is "ACTIVE", otherwise, returns False.
def detect_start(device):
	Execution = []
	
	try:
	    AgentAddress = 'http://192.168.1.254:5000/current'
	except:
	    print("Cannot connect to internet / agent address")

	try:
	    xml = urllib2.urlopen(AgentAddress)
	    xml = urllib2.urlopen(AgentAddress)
	    xml = xml.read()
	    data = ET.ElementTree(ET.fromstring(xml))
	    test = data.getroot()
	    yolo = test.findall('.//')
	except urllib2.URLError:
	    print("Invalid MTConnect Agent Address! Please connect device to internet.")
	    GPIO.output(22, False)
	
	try:
	    for yolos in yolo:
		if (yolos.tag == '{urn:mtconnect.org:MTConnectStreams:1.3}Execution'):
		    Execution.append(yolos.text)

	    print("Execution mode for " + device + " is: " + str(Execution[0]))
	    if(Execution[0] == ACTIVE):
                return True
            else:
                return False
	except:
	    return False 


# This function returns False if we reach the time where the last tool started
def detect_end(device, time_since_start):
    device_list = float_toolID_timestamp_reference.get(device) 
    last_element_time = device_list[len(device_list) - 1][1]

    if ( time_since_start > last_element_time ):
        return False 
    else:
        return True

# This function parse a timestamp string and convert it into a float number (unit is seconds)
# i.e. [[1, "00:00:25.005"], [1, "00:00:35.005"]] becomes [[1, 25.005], [1, 35.005]]
def parse_string(device):
    list = []

    try:
        for item in toolID_timestamp_reference[device]:
	    parse_timestamp = item[1].split(":")
	    hour = int(parse_timestamp[0])
	    minutes = int(parse_timestamp[1])
	    seconds = float(parse_timestamp[2])
            tool_time = hour * 3600 + minutes * 60 + seconds
	    list.append([item[0], tool_time])
        float_toolID_timestamp_reference[device] = list

    except:
        print("\n Error: Cannot parse the timestamp string! \n") 


#============================ get_toolID FUNCTION ====================================================================
# Given the time since start, this function returns the toolID that is currently running
def get_toolID(device, time_since_start): 

    try:
        for i in range(len(float_toolID_timestamp_reference[device])):	
            for item in float_toolID_timestamp_reference[device]:
                if (time_since_start <= item[1]):
		
		    #print("\nCurrently running tool is tool " + str(item[0]))
                    return item[0]
                else:
                    continue    	
    except:
        return 1

#=============================== MAIN CODE =============================================================================


def update_toolID(device, toolID):
	tools="""SELECT DISTINCT Tool_ID from Feature_Dictionary WHERE MAC_ADDRESS=(%s)"""
	x.execute(tools,[device])
	tool_IDs = x.fetchall()
	tool_IDs = list(list(zip(*tool_IDs))[0])
	if toolID not in tool_IDs:
	    newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Tool_ID, Tool_Status, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Device_Status, Machine_Name) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,(device, toolID, "1", "0","Signal Energy", "30","600","1200", "1", device))
            newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Tool_ID, Tool_Status, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Device_Status, Machine_Name) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device, toolID, "1", "1","Mean Crossing Rate", "100","500","1000", "1", device])
	    newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Tool_ID, Tool_Status, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Device_Status, Machine_Name) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device, toolID, "1", "2","Spectral Centroid", "6","7","8", "1", device])
	    newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Tool_ID, Tool_Status, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Device_Status, Machine_Name) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device, toolID, "1", "3","Spectral Flatness", "40","42","45", "1", device])
            newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Tool_ID, Tool_Status, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Device_Status, Machine_Name) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device, toolID, "1", "4","Accel Spectral Spread", "200","205","210", "1", device])
	    newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Tool_ID, Tool_Status, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Device_Status, Machine_Name) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device, toolID, "1", "5","Audio Spectral Spread", "500","1000","1500", "1", device])
	    toolstatusupdate = """UPDATE Feature_Dictionary SET Tool_Status = (%s) WHERE MAC_ADDRESS=(%s) AND Tool_ID <> (%s)"""
	    x.execute(toolstatusupdate,('0',device,toolID))
	else:
	    toolstatusonupdate = """UPDATE Feature_Dictionary SET Tool_Status = (%s) WHERE MAC_ADDRESS=(%s) AND Tool_ID = (%s)"""
	    x.execute(toolstatusonupdate,('1',device,toolID))
	    toolstatusoffupdate = """UPDATE Feature_Dictionary SET Tool_Status = (%s) WHERE MAC_ADDRESS=(%s) AND Tool_ID <> (%s)"""
	    x.execute(toolstatusoffupdate,('0',device,toolID))
	conn.commit()


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



global x
GPIO.setmode(GPIO.BOARD)
GPIO.setup(22,GPIO.OUT)
GPIO.setup(32,GPIO.OUT)
GPIO.setup(36,GPIO.OUT)
GPIO.setup(40,GPIO.OUT)
GPIO.output(22, True)
try:
    conn = MySQLdb.connect(host="173.194.252.192", user="infinite-uptime", passwd="infiniteuptime", db="Hub_Dictionary")
    x =conn.cursor()
except:
    print("Unable to access hub dictionary.")
    GPIO.output(32, False)
    GPIO.output(40, False)
    raise

# Determine which database this Pi corresponds to.
try:
    serial = getserial()
    x.execute("SELECT Database_Name FROM Hub_Mapping WHERE Hub_ID='%s'" % serial)
    db_name = x.fetchall()[0][0]
except:
    report_error("Could not find database for serial number: " + serial)
    GPIO.output(32, False)
    raise

# Switch connection to the appropriate database, if found.
try:
    conn = MySQLdb.connect(host="173.194.252.192", user="infinite-uptime", passwd="infiniteuptime", db=db_name)
    x =conn.cursor()
except:
    report_error("Connection to database failed.")
    GPIO.output(32, False)
    raise



# initialize later used variables
tool_ID={}
started = {}
initial_time = {}
x.execute("SELECT DISTINCT MAC_ADDRESS FROM Feature_Dictionary WHERE Device_Status = '1'")
devices = x.fetchall()
devices = list(list(zip(*devices))[0])
for device in devices:
    started[device]= False
    initial_time[device] = 0
    parse_string(device)


while True:
    x.execute("SELECT DISTINCT MAC_ADDRESS FROM Feature_Dictionary WHERE Device_Status = '1'")
    devices = x.fetchall()
    devices = list(list(zip(*devices))[0])

# If a device is not started, continuesly checking whether the device is started .
# if a device is started, get current tool ID number and update.   
    for device in devices:       
	if(not started[device]):
	    try:
	        started[device] = detect_start(device)
	        initial_time[device] = get_currentTime()
            except:
	        print("Error: Having trouble detecting the start time for " + str(device))
        if(started[device]):
            try:
	        time_since_start = get_currentTime()-initial_time[device]
                print("\n time since start is: " + str(time_since_start) + " for " + str(device))
                toolid = get_toolID(device, time_since_start)
                if (toolid != None):
                    print("wrote tool ID as " + str(toolid) + " for " + device)
	            update_toolID(device,toolid)
	        started[device] = detect_end(device, time_since_start)            
            except:
                print('Tool ID update failed for ' + str(device))
                GPIO.output(22, False)
	        raise
