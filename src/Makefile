CC = g++ -c
CCL = g++

OBJECTS = main.o bst.o client.o clientBuffer.o clientIndex.o \
	exploration.o gui.o gui_about.o hash.o server.o serverIndex.o sha.o

LINKER_OPTIONS = `pkg-config gtkmm-2.4 --cflags --libs` \
	-lboost_thread -D_REENTRANT -lboost_filesystem 

main: $(OBJECTS)
	$(CCL) $(OBJECTS) -o ../simple-p2p $(LINKER_OPTIONS)
	@echo "\nBuilding simple-p2p completed!\n"

main.o: main.cc gui.o
	$(CC) main.cc `pkg-config gtkmm-2.4 --cflags`

bst.o: bst.cc bst.h globals.h
	$(CC) bst.cc

client.o: client.cc client.h globals.h clientBuffer.o clientIndex.o
	$(CC) client.cc

clientBuffer.o: clientBuffer.cc clientBuffer.h globals.h bst.o hash.o
	$(CC) clientBuffer.cc

clientIndex.o: clientIndex.cc clientIndex.h globals.h
	$(CC) clientIndex.cc

exploration.o: exploration.cc exploration.h globals.h
	$(CC) exploration.cc

gui.o: gui.cc gui.h globals.h client.o gui_about.o server.o
	$(CC) gui.cc `pkg-config gtkmm-2.4 --cflags`

gui_about.o: gui_about.cc gui_about.h globals.h
	$(CC) gui_about.cc `pkg-config gtkmm-2.4 --cflags`

hash.o: hash.cc hash.h globals.h sha.o
	$(CC) hash.cc

server.o: server.cc server.h globals.h serverIndex.o
	$(CC) server.cc

serverIndex.o: serverIndex.cc serverIndex.h globals.h
	$(CC) serverIndex.cc

sha.o: sha.cc sha.h
	$(CC) sha.cc

clean:
	rm -v *.o

#builds a program to test out the binary search tree
bst_test: bst.cc bst_test.cc
	$(CCL) bst.cc bst_test.cc  -o bst_test

#builds a program to test out SHA2 hashing
sha_test: sha.cc sha_test.cc
	$(CCL) sha.cc sha_test.cc -o sha_test
