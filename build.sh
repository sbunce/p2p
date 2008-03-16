#!/bin/bash

if [ `ls -1 download/ | wc -l` != 0 ]
then
	rm download/*
fi

cd src
make -j2

if [ `ls -l |grep -c p2p*` != 0 ]
then
	cp p2p* ..
fi

exit 0
