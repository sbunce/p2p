all:
	cd $(shell pwd)
	scons

clean:
	cd $(shell pwd)
	scons -c
