import time
import os
import signal

p = os.system("sudo hciconfig hci0 down; sudo hciconfig hci0 up")
q = os.system("hcitool lescan > scan.txt & sleep 2; pkill --signal SIGINT hcitool")
r = os.system("sudo close(scan.txt)")

content=[]
with open('scan.txt','r') as f:
    for line in f:
	content.append(line)

content=map(str.strip,content)

addr=[]
for line in content:
    if line.find('HMSoft') >= 0:
	templine=line.split('HMSoft')
	templine[0]=templine[0].strip()
	addr.append(templine[0])

print addr