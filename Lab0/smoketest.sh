#!/bin/bash
#NAME: Ethan Ngo
#EMAIL: ethan.ngo2019@gmail.com
#ID: 205416130

# #Test 1
	# echo "Basic I/O test" > input.txt
	# ./lab0 -i input.txt -o output.txt
	# cmp input.txt output.txt
# 	if[ $? -ne 0 ]
# 	then
# 		echo "Basic I/O test failed!" > errorLog.txt
# 	fi
# 	rm -f input.txt output.txt

# 	#Test 2
# 	./lab0 -o output.txt
# 	PID=$!
# 	echo "Basic output from stdin test"
# 	kill $PID
# 	echo "Basic output from stdin test" > compare.txt
# 	cmp compare.txt output.txt
# 	if[ $? -ne 0 ]
# 	then
# 		echo "Basic output from stdin test failed!"  > errorLog.txt
# 	fi
# 	rm -f compare.txt output.txt

# 	#Test 3
# 	#this file must not exist in the user's system, which I highly doubt
# 	./lab0 -i thereisnowaythisfileexists.txt
# 	if[ $? -ne 2 ]
# 	then
# 		echo "Input of nonexistent file failed!"  > errorLog.txt
# 	fi

# 	#Test 4
# 	touch badFile.txt
# 	chmod -w badFile.txt
# 	if[ $? -ne 3 ]
# 	then
# 		echo "Unwrittable file test failed!"  > errorLog.txt
# 	fi
# 	rm -f badFile.txt
	

# 	#Test 5
# 	echo "Argument Test..."
# 	./lab0 --bad
# 	if[ $? -ne 1 ]
# 	then
# 		echo "Invalid Argument Test Failed!"  > errorLog.txt
# 	fi

# 	#Test 6
# 	echo "Catch Test..."
# 	./lab0 -s -c
# 	if [ $? -ne 4 ]
# 	then
# 		echo "Catch test failed!"  > errorLog.txt
# 	fi

# 	Test 7
	# echo "Segmentation Test..."
	# ./lab0 --segfault
	# if[ $? -eq 139 ]
	# then
	# 	echo "We good"
	# else 
	# 	echo "Segmentation Fault test failed!" 
	# fi

# 	touch empty.txt
# 	cmp empty.txt errorLog.txt
# 	if[ $? -eq 0 ]
# 	then
# 		echo "Smoke tests passed!"
# 	fi

# 	rm -f empty.txt errorLog.txt