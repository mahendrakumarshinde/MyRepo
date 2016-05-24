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

def get_toolID(device):

	Toolid = []
	
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
		if (yolos.tag == '{urn:mtconnect.org:MTConnectStreams:1.3}ToolId'):
		    Toolid.append(yolos.text)

	    print("Tool ID is: " + str(Toolid[0]))
	    return int(Toolid[0])
	except:
	    return 1

def update_toolID(device, toolID):
	tools="""SELECT DISTINCT Tool_ID from Feature_Dictionary WHERE MAC_ADDRESS=(%s)"""
	x.execute(tools,[device.deviceAddr])
	tool_IDs = x.fetchall()
	tool_IDs = list(list(zip(*tool_IDs))[0])
	if tool_ID[device.deviceAddr] not in tool_IDs:
	    newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Tool_ID, Tool_Status, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Device_Status, Machine_Name) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,(device.deviceAddr, tool_ID[device.deviceAddr], "1", "0","Signal Energy", "30","600","1200", "1", device.deviceAddr))
            newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Tool_ID, Tool_Status, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Device_Status, Machine_Name) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device.deviceAddr, tool_ID[device.deviceAddr], "1", "1","Mean Crossing Rate", "100","500","1000", "1", device.deviceAddr])
	    newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Tool_ID, Tool_Status, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Device_Status, Machine_Name) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device.deviceAddr, tool_ID[device.deviceAddr], "1", "2","Spectral Centroid", "6","7","8", "1", device.deviceAddr])
	    newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Tool_ID, Tool_Status, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Device_Status, Machine_Name) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device.deviceAddr, tool_ID[device.deviceAddr], "1", "3","Spectral Flatness", "40","42","45", "1", device.deviceAddr])
            newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Tool_ID, Tool_Status, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Device_Status, Machine_Name) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device.deviceAddr, tool_ID[device.deviceAddr], "1", "4","Accel Spectral Spread", "200","205","210", "1", device.deviceAddr])
	    newDeviceThresholds = """INSERT INTO Feature_Dictionary (MAC_ADDRESS, Tool_ID, Tool_Status, Feature_ID, Feature_Name, Threshold_1, Threshold_2, Threshold_3, Device_Status, Machine_Name) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"""
	    x.execute(newDeviceThresholds,[device.deviceAddr, tool_ID[device.deviceAddr], "1", "5","Audio Spectral Spread", "500","1000","1500", "1", device.deviceAddr])
	    toolstatusupdate = """UPDATE Feature_Dictionary SET Tool_Status = (%s) WHERE MAC_ADDRESS=(%s) AND Tool_ID <> (%s)"""
	    x.execute(toolstatusupdate,('0',device,tool_ID[device.deviceAddr]))
	else:
	    toolstatusonupdate = """UPDATE Feature_Dictionary SET Tool_Status = (%s) WHERE MAC_ADDRESS=(%s) AND Tool_ID = (%s)"""
	    x.execute(toolstatusonupdate,('1',device.deviceAddr,tool_ID[device.deviceAddr]))
	    toolstatusoffupdate = """UPDATE Feature_Dictionary SET Tool_Status = (%s) WHERE MAC_ADDRESS=(%s) AND Tool_ID <> (%s)"""
	    x.execute(toolstatusoffupdate,('0',device.deviceAddr,tool_ID[device.deviceAddr]))
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

while true:
    try:
        x.execute("SELECT DISTINCT MAC_ADDRESS FROM Feature_Dictionary)
        devices = x.fetchall()
	devices = list(list(zip(*devices))[0])
	for device in devices:
	    toolid = get_toolID(device)
	    update_toolID(device,toolid)
    except:
        print('Tool ID update failed')
        GPIO.output(22, False)
	raise