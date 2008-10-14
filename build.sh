#!/bin/sh
#This is a convenience script to run scons and copy the built programs to the
#top program directory.

TOP_DIR=`pwd`

cd src
scons
cd $TOP_DIR
cp src/p2p_gui ./
cp src/p2p_nogui ./
