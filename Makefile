CXXFLAGS = -O3 -Werror -ansi
CFLAGS = -O3 -Werror -ansi
CPU_COUNT = `cat /proc/cpuinfo |grep -c "processor"`
#CXXFLAGS = -pg

#sub-makefiles to export variables to
.EXPORT_ALL_VARIABLES: src

.PHONY: all
all: src
	cp src/p2p ./
	cp src/p2p_server ./
	@echo "building complete"

#make main source tree
.PHONY: src
src:
	$(MAKE) -j$(CPU_COUNT) -C src

#remove all object files
.PHONY: clean
clean:
	$(MAKE) -j$(CPU_COUNT) -C src clean
	rm -f hash/*
	rm -f download/*
