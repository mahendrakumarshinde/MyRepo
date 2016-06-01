import xml.etree.ElementTree as ET
import urllib2
import sys
import time
import re


def get_toolID(device):

	Toolid = []
	
	try:
	    AgentAddress = 'http://192.168.1.254:5000/current'
	except:
	    print("Cannot connect to internet / agent address")

	try:
	    xml = urllib2.urlopen(AgentAddress)
	    xml = xml.read()
	    data = ET.ElementTree(ET.fromstring(xml))
	    test = data.getroot()
	    yolo = test.findall('.//')
	except urllib2.URLError:
	    print("Invalid MTConnect Agent Address! Please connect device to internet.")

	
	try:
	    for yolos in yolo:
		if (yolos.tag == '{urn:mtconnect.org:MTConnectStreams:1.3}ToolId'):
		    Toolid.append(yolos.text)

	    print("Tool ID is: " + str(Toolid[0]) + " for " + str(device))
	    if(Toolid[0] == "UNAVAILABLE"):
                return 1
            else:
                return int(Toolid[0])
              
	except:
	    print("Couldn't extract Tool ID, returning default Tool ID 1 for " + str(device.deviceAddr))
	    return 1