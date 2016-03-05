import RPi.GPIO as g
import btle
import time


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
        btle.DefaultDelegate.__init__(self)
    
    def handleNotification(self, cHandle, data):
        data= data.replace("\r\n","")
        
        value = int(data)
        print(value)
        if (value < 300):
            print("GREEN")
            g.output(14, g.HIGH)
            g.output(15, g.HIGH)
            g.output(18, g.LOW)
        elif (value < 500):
            print("ORANGE")
            g.output(14, g.HIGH)
            g.output(15, g.LOW)
            g.output(18, g.HIGH)
        else:
            print("RED")
            g.output(14, g.LOW)
            g.output(15, g.HIGH)
            g.output(18, g.HIGH)

p = btle.Peripheral("54:4A:16:26:A5:A4")
p.setDelegate(TeensyDelegate())
print "Initializing and Connecting..."

while True:
    if p.waitForNotifications(1.0):
        print "Received data"
        continue

    print "Waiting..."