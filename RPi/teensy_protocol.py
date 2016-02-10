import RPi.GPIO as g
import btle
import time
import datetime

SENSOR_OUTPUT = "sensor-data.txt"

g.setmode(g.BCM)
g.setup(14, g.OUT)
g.setup(15, g.OUT)
g.setup(18, g.OUT)

g.output(14, g.HIGH)
g.output(15, g.HIGH)
g.output(18, g.HIGH)

for i in range(5):
    g.output(18, g.HIGH)
    g.output(14, g.LOW)
    time.sleep(0.1)
    g.output(14, g.HIGH)
    g.output(15, g.LOW)
    time.sleep(0.1)
    g.output(15, g.HIGH)
    g.output(18, g.LOW)
    time.sleep(0.1)
    
g.output(14, g.HIGH)
g.output(15, g.HIGH)
g.output(18, g.HIGH)

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
	    print(msg)
            msg= msg.replace("\r\n","")
            print(msg)
            #value = int(data)
            msg = msg.split(",")
	    print(msg)
	    print(len(msg))
	    date = str(datetime.datetime.now())
	    f = open(SENSOR_OUTPUT, "a")
	    f.write("[" + date + "] " + ' '.join(msg) + '\n')
	    f.close()	
#	print("MAC address: " + data[0])
#	print("State: " + data[1])
#        features = {}
#        #data = data[2:]
#	while (data):
#	    key = data.pop(0)
#	    val = data.pop(0)
#            features[key] = val
#	print(features)
#        if (value < 300):
#            print("GREEN")
#            g.output(14, g.HIGH)
#            g.output(15, g.HIGH)
#            g.output(18, g.LOW)
#        elif (value < 500):
#            print("ORANGE")
#            g.output(14, g.HIGH)
#            g.output(15, g.LOW)
#            g.output(18, g.HIGH)
#        else:
#            print("RED")
#            g.output(14, g.LOW)
#            g.output(15, g.HIGH)
#            g.output(18, g.HIGH)

p1 = btle.Peripheral("68:9E:19:07:DE:98")
p2 = btle.Peripheral("68:9E:19:07:F2:47")
p1.setDelegate(TeensyDelegate())
p2.setDelegate(TeensyDelegate())
print "Initializing and Connecting..."

while True:
    if p1.waitForNotifications(0.1):
        print "Received data 1"
    if p2.waitForNotifications(0.01):
        print "Received data 2"
    print "Waiting..."
