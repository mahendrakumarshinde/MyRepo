import xml.etree.ElementTree as ET
import urllib2
import sys
import time
import re


def get_toolID():

	Toolid = []
	
	AgentAddress = 'http://192.168.1.254:5000/current'

	try:
	    xml = urllib2.urlopen(AgentAddress)
	except urllib2.URLError:
		sys.exit("Invalid MTConnect Agent Address! Please connect device to internet.")

	xml = urllib2.urlopen(AgentAddress)
	xml = xml.read()
	data = ET.ElementTree(ET.fromstring(xml))
	test = data.getroot()
	yolo = test.findall('.//')
	for yolos in yolo:
		if (yolos.tag == '{urn:mtconnect.org:MTConnectStreams:1.3}ToolId'):
			Toolid.append(yolos.text)

	print(Toolid[0])



get_toolID()