.PHONY:
all: dot_gen
	cd .. && ./graph/dot_gen main.cc |dot -Tps -o graph/p2p.ps
	cd .. && ./graph/dot_gen main_server.cc |dot -Tps -o graph/p2p_nogui.ps

dot_gen: main.o scan.o
	$(CXX) -o dot_gen main.o scan.o

main.o: main.cc
scan.o: scan.cc scan.h

.PHONY:
clean:
	rm -f dot_gen *.ps *.o
