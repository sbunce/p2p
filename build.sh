#!/bin/bash

if [ `ls -1 download/ | wc -l` != 0 ]
then
	rm download/*
fi

cd src
make -j2

exit 0

