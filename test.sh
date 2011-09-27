#!/bin/sh

rpoly() {
	./poly $2 $3 $4 $5 $6
	python save.py
	mv viewbuf.txt $1
}

#rpoly test1.txt 100.0 100.0 -2.5 1.5 -2.0
#rpoly test1a.txt 100.0 100.0 -1.5 1.5 -2.0
#rpoly test1b.txt 100.0 100.0 -3.5 1.5 -2.0
#rpoly test1c.txt 100.0 100.0 -2.5 0.5 -2.0
rpoly test1d.txt 3.0 3.0 25.5 -123.5 15.0
#rpoly test2.txt 20.0 20.0 -2.5 1.5 -2.0

