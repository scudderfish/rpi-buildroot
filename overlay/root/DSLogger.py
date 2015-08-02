#!/usr/bin/env python
import sys, string, os, subprocess,time

startTime = time.time()*1000

filename=time.strftime("/root/temperature-%Y%m%d%H%M%S.csv",time.gmtime())
f=open(filename,'w+')
ids=[];
#Get ids

r=open("/sys/bus/w1/devices/w1_bus_master1/w1_master_slaves")

for x in r.readlines():
	ids.append(x.strip())

f.write('time,device,value\n')
while 1 != 2:
	for id in ids:
		data=open("/sys/bus/w1/devices/w1_bus_master1/{0}/w1_slave".format(id)).readlines()
		if(data[0].find('YES')):
			value=data[1].split("=")[1]
			now=time.time()*1000
			f.write('{0},{1},{2:.2f}\n'.format(int(now-startTime),id,float(value)/1000))
			f.flush()
		
		
		
