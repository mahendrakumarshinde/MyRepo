import RPi.GPIO as g
import btle
import time
import datetime

# Output file for sensor data
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

"""
This is a basic protocol for gathering data from Teensys.

The following delimiters are in use:
    ';' delimits a message
    ',' delimits fields in a message (state, feature ID, etc)

The format of each message is currently:

{MAC ADDRESS},{STATE},{FEATURE-ID-1},{FEATURE-VALUE-1}....{FEATURE-ID-N},{FEATURE-VALUE-N};

It appears that the Teensy sends data in 20-character chunks.
Thus, we keep a buffer and append 20-character chunks until a ';' is found.
"""

# Subclass of Default Delegate. Responds whenever data is sent from a teensy to this RPi.
class TeensyDelegate(btle.DefaultDelegate):
    def __init__(self):
        # Empty data buffer persists over multiple 20-character messages
	    self.buffer = ""
        btle.DefaultDelegate.__init__(self)
    
    # Handles data sent from the Teensy. Print statements present from debugging.
    def handleNotification(self, cHandle, data):

        # Data appended to the buffer
    	self.buffer = self.buffer + data

        # If a complete message is present (delimited by ';'): process and store with a timestamp
    	if (";" in self.buffer):
    	    two = self.buffer.split(";");

            # Resave partial message
    	    self.buffer = two[1]
            # Process whole message
    	    msg = two[0]

    	    print(msg)
            msg = msg.replace("\r\n","")
            print(msg)

            # Split comma-delimited values
            msg = msg.split(",")
    	    print(msg)
    	    print(len(msg))

            # Produce a timestamp
    	    date = str(datetime.datetime.now())

            # Append to file using "a" for "append"
    	    f = open(SENSOR_OUTPUT, "a")
    	    f.write("[" + date + "] " + ' '.join(msg) + '\n')
    	    f.close()

# Initialize a Peripheral object corresponding to each Teensy.
# Plan is to interface this with LEscan to automate MAC address finding.
p1 = btle.Peripheral("68:9E:19:07:DE:98")
p2 = btle.Peripheral("68:9E:19:07:F2:47")
p1.setDelegate(TeensyDelegate())
p2.setDelegate(TeensyDelegate())
print "Initializing and Connecting..."

while True:
    # up to 100 Hz data collection tested, no problems seen
    if p1.waitForNotifications(0.01):
        print "Received data 1"
    if p2.waitForNotifications(0.01):
        print "Received data 2"
    print "Waiting..."
