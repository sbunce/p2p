#!/bin/sh

TOP_DIR=`pwd`

if [ "$1" == "clean" ]; then
	#clean up
	cd src
	scons -c
	cd $TOP_DIR
	rm -f p2p_gui p2p_nogui
	rm -f download/*
	rm -f hash/*
else
	#build
	cd src
	scons -j2
	cd $TOP_DIR
	cp src/p2p_gui ./
	cp src/p2p_nogui ./
fi
