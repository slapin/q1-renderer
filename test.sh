#!/bin/bash

rpoly() {
	./poly-null $2 $3 $4 $5 $6 || {
		echo $2 $3 $4 $5 $6 - $? >> fault.log
		gdb --args ./poly $2 $3 $4 $5 $6 
		return
	}
#	python save.py
#	mv viewbuf.txt $1.txt
	[ -r viewbuf ] && convert -size 1280x1024 -depth 8 gray:viewbuf -depth 24 -size 1280x1024 $1.png
}

#for ((i = -50; i < 10; i = i + 4)); do
#	for ((j = -20; j < 10; j++)); do
#		for ((k = -10; k < 0; k = k + 2)); do
#			rpoly test_$i+$j+$k 100.0 100.0 $i $j $k
#		done
#	done
#done
#exit 0

#rpoly test1 100.0 100.0 -2.5 1.5 -2.0
#rpoly test1a 100.0 100.0 -1.5 1.5 -2.0
#rpoly test1b 100.0 100.0 -3.5 1.5 -2.0
#rpoly test1c 100.0 100.0 -2.5 0.5 -2.0
rpoly test1d 3.0 3.0 25.5 -123.5 15.0
#rpoly test2 20.0 20.0 -2.5 1.5 -2.0

