#!/bin/sh

TOP_DIR=`pwd`

if [ "$1" == "clean" ]; then
	#clean up
	clear
	cd src
	python scons/scons.py -c
	cd $TOP_DIR
	rm -f p2p_gui p2p_nogui
	rm -f download/*
else
	#build
	clear
	cd src
	python scons/scons.py
	cd $TOP_DIR
	cp src/p2p_gui ./
	cp src/p2p_nogui ./
fi
