#!/usr/bin/env python
import sys, string, os, subprocess,time

startTime = time.time()*1000

ids=[];
#Get ids
a=subprocess.Popen(['DS18B20','-gpio','4'],stdout=subprocess.PIPE)
for line in iter(a.stdout.readline,''):
	ids.append(line.rstrip().split()[0])

print 'timestamp,' + ','.join(ids)
while 1 != 2:
	data={}
	a=subprocess.Popen(['DS18B20','-gpio','4'],stdout=subprocess.PIPE)
	for line in iter(a.stdout.readline,''):
		dataLine=line.rstrip()
		items=dataLine.split()
		data[items[0]]=items[5];
	
	outputLine=[]
	currentTime = round(time.time()*1000 - startTime,0)
	outputLine.append(str(currentTime))	
	dataOK=True
	for id in ids:
		if id in data.keys():
			outputLine.append(data[id])
		else:
			dataOK=False;
	if dataOK:
		print ','.join(outputLine)
	else:
		print data
