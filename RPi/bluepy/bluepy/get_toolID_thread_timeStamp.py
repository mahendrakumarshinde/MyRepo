import xml.etree.ElementTree as ET
import urllib2
import sys
import re
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




def get_ToolTime(toolID_timestamp_array):

    for n in range(len(toolID_timestamp_array)):

        # parse the timestamp string, then calculate the value in millisecond
        # i.e. [[2, '00:45:23'], [1, '23:11:01']] becomes [[2, 45023], [1,1391001]]
        try:
            parse_timestamp = toolID_timestamp_array[n][1].split(":")
            parse_timestamp = [int(i) for i in parse_timestamp]
            parse_timestamp = parse_timestamp[0] * 60000 + parse_timestamp[1] * 1000 + parse_timestamp[2]
            toolID_timestamp_array[n][1] = parse_timestamp
        except:
            print("Having trouble parsing the timestamp string!")

    # calculate the time needed for running each tool
    toolID = []
    timestamp = []
    time_for_each_tool = []
    result = []

    try:
        # put toolID and timestamp into two different lists
        for i in range(len(toolID_timestamp_array)):
            toolID.append(toolID_timestamp_array[i][0])
            timestamp.append(toolID_timestamp_array[i][1])
    except:
        print("Having trouble splitting the input list into two separate lists!")


    try:
        # get time difference between two adjencent tools
        for i in range (len(timestamp)):
            next_i = i+1
            if (next_i < len(timestamp)):
                time_for_each_tool.append(timestamp[next_i] - timestamp[i])
        time_for_each_tool.append("")
    except:
        print("Having trouble find the time difference!")


    try:
        # Now, create a list that contains all the tools with its corresponding running time
        # Here, we use set() because it will remove duplicates automatically
        result = set(zip(toolID, time_for_each_tool))
        result = dict((y,x) for x, y in result)

    except:
        print("Having trouble 'zip' the toolID list and time difference list!")


    print(result) # print to show the result since "result" is not visible outside the function
    return result

# uncomment below to test the function
# get_ToolTime([[2, "00:00:12"], [1, "00:00:25"], [1, "00:00:39"], [3, "00:00:53"]])



#================================================================================================

def get_toolID(toolID_timestamp_array, tool_running_time):
    toolID_dict = get_ToolTime(toolID_timestamp_array)
    toolID = toolID_dict.get(tool_running_time)
    print(toolID)
    return toolID


# uncomment below to test the function
# get_toolID([[2, "00:00:12"], [1, "00:00:25"], [1, "00:00:39"], [3, "00:00:53"]], 13)


#================================================================================================

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

tool_ID={}

while True:
    try:
        x.execute("SELECT DISTINCT MAC_ADDRESS FROM Feature_Dictionary")
        devices = x.fetchall()
	devices = list(list(zip(*devices))[0])
	for device in devices:
	    #activetool="""SELECT DISTINCT Tool_ID FROM Feature_Dictionary WHERE MAC_ADDRESS = (%s) AND Tool_Status = (%s)"""
	    #toolactive = x.execute(activetool,(device,'1'))
	    #toolactive = x.fetchall()[0][0]
	    #tool_ID[device] = toolactive
	    toolid = get_toolID([[2, "00:00:12"], [1, "00:00:25"], [1, "00:00:39"], [3, "00:00:53"]], 13)
	    print("wrote tool ID as " + str(toolid) + " for " + device)
	    update_toolID(device,toolid)
	    
    except:
        print('Tool ID update failed')
        GPIO.output(22, False)
	raise

