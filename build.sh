#!/bin/bash

if [ `ls -1 download/ | wc -l` != 0 ]
then
	rm download/*
fi

cd src

if [ "$1" == "clean" ]
then
	make clean
else
	make -j2
	make -j2 server
fi

exit 0

