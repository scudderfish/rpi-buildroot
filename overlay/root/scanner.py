#!/usr/bin/env python

import sys,os
from lcd import lcd 


display=lcd.lcd()
display.lcd_clear()

def reportFile(path):
	display.lcd_clear()
	display.lcd_display_string("Found",1)	
	display.lcd_display_string(path,2)

def checkSig(path):
	sigOK=False
	returnValue = os.system("gpgv %s" % path)
	sigOK = (returnValue == 0)
	if not sigOK:
		display.lcd_clear()
		display.lcd_display_string("Bad sig",1)
		display.lcd_display_string(path,2)
		
	return sigOK
	
def processCommand(path):
	reportFile(path)
	if not checkSig(path):
		return
	sys.exit(0)	

def processKey(path):
	reportFile(path)
	if not checkSig(path):
		return
	sys.exit(0)

def scanUSB():
	ROOT='/media'
	KEY_FILE='key.asc'
	COMMAND_FILE='command.asc'
	roots=os.listdir(ROOT);
	display.lcd_display_string("Insert USB key",1)

	while True:
		for dir in roots:
			testPath='%s/%s/%s' % (ROOT,dir,KEY_FILE)
			if os.path.isfile(testPath):
				processKey(testPath)
			testPath='%s/%s/%s' % (ROOT,dir,COMMAND_FILE)
			if os.path.isfile(testPath):
				processCommand(testPath)
			
if __name__ == "__main__":
	scanUSB()