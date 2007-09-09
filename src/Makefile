#build packages needed g++ libboost-thread-dev libboost-filesystem-dev libsqlite3-dev

CC = g++ -O3 -c
CCL = g++

#GPROF = -pg

OBJECTS = main.o client.o clientBuffer.o download.o clientIndex.o \
	exploration.o gui.o gui_about.o hash.o server.o serverIndex.o sha.o \
	superBlock.o

LINKER_OPTIONS = `pkg-config gtkmm-2.4 --cflags --libs` -lboost_filesystem \
	-lboost_thread -D_REENTRANT -lsqlite3 

SERVER_OBJECTS = main-server.o hash.o server.o serverIndex.o sha.o

SERVER_LINKER_OPTIONS = -lboost_filesystem -lboost_thread -D_REENTRANT \
	-lsqlite3

main: $(OBJECTS)
	$(CCL) $(OBJECTS) $(GPROF) -o ../simple-p2p $(LINKER_OPTIONS)
	@echo "\nBuilding simple-p2p complete.\n"

main.o: main.cc gui.o
	$(CC) main.cc `pkg-config gtkmm-2.4 --cflags`

main-server.o: main-server.cc server.o
	$(CC) main-server.cc

client.o: client.cc client.h clientBuffer.o clientIndex.o exploration.o global.h 
	$(CC) client.cc

clientBuffer.o: clientBuffer.cc clientBuffer.h download.o global.h
	$(CC) clientBuffer.cc

clientIndex.o: clientIndex.cc clientIndex.h download.o exploration.o global.h
	$(CC) clientIndex.cc

download.o: download.cc download.h hash.o superBlock.o global.h
	$(CC) download.cc

exploration.o: exploration.cc exploration.h global.h
	$(CC) exploration.cc

gui.o: gui.cc gui.h client.o exploration.o gui_about.o server.o global.h
	$(CC) gui.cc `pkg-config gtkmm-2.4 --cflags`

gui_about.o: gui_about.cc gui_about.h global.h
	$(CC) gui_about.cc `pkg-config gtkmm-2.4 --cflags`

hash.o: hash.cc hash.h sha.o global.h
	$(CC) hash.cc

server.o: server.cc server.h serverIndex.o global.h
	$(CC) server.cc

serverIndex.o: serverIndex.cc serverIndex.h global.h
	$(CC) serverIndex.cc

sha.o: sha.cc sha.h global.h
	$(CC) sha.cc

superBlock.o: superBlock.cc superBlock.h global.h
	$(CC) superBlock.cc

sha_test: sha_test.cc sha.o
	$(CCL) sha_test.cc sha.cc -o ../sha-test

server: $(SERVER_OBJECTS)
	$(CCL) $(SERVER_OBJECTS) $(GPROF) -o ../server $(SERVER_LINKER_OPTIONS) 
	@echo "\nBuilding server complete.\n"

clean:
	rm -v *.o

