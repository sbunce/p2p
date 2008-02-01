#!/bin/bash

if [ `ls -1 download/ | wc -l` != 0 ]
then
	rm download/*
fi

cd src

if [ "$1" == "clean" ]
then
	make clean
elif [ "$1" == "server" ]
then
	make -j2 server
else
	make -j2
fi

exit 0

