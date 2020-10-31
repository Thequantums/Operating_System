#!/bin/bash

# test the standard arguments

./lab4b --period=1 --scale="C" --log="LOGFILE" <<-EOF
SCALE=F
PERIOD=1
START
STOP
LOG test
OFF
EOF
ret=$?
if [ $ret -ne 0 ]
then
	echo "Test failed: return is not zero."
else 
	echo "Test Passed: return is zero."
fi

if [ ! -s LOGFILE ]
then
	echo "Test failed: no LOGFILE created"
else
	echo "Test passed: LOGFILE was created"
	grep "SCALE=F" LOGFILE > /dev/null
		if [ $? -ne 0 ]
		then
			echo "DID NOT LOG SCALE=F command"
		else
			echo "    SCALE=F ... RECOGNIZED AND LOGGED"
		fi

	grep "SHUTDOWN" LOGFILE > /dev/null
		if [ $? -ne 0 ]
		then
			echo "DID NOT LOG SHUTDOWN command"

		else
			echo "    SHUTDOWN ... RECOGNIZED AND LOGGED"
		fi	
	
fi

echo "CHECKING correct reporting format"
egrep '[0-9][0-9]:[0-9][0-9]:[0-9][0-9] [0-9]+\.[0-9]\>' LOGFILE > FOUND
if [ $? -eq 0 ] 
then
	echo "Test Passed: correct format report"
else
	echo "Test failed: wrong format"
fi
