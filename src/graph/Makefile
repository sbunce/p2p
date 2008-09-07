all:
	$(shell g++ -o dot_gen main.cc && cat ../.depend |./dot_gen |dot -Tps -o p2p.ps)

clean:
	rm -f dot_gen p2p.ps *.o
