CXXFLAGS = -O3 -Werror -ansi
CFLAGS = -O3 -Werror -ansi
#GPROF = -pg

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
	$(MAKE) -j2 -C src

#remove all object files
.PHONY: clean
clean:
	$(MAKE) -C src clean
