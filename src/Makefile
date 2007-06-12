CC = g++ -O3 -c
CCL = g++

OBJECTS = main.o client.o download.o clientIndex.o exploration.o gui.o \
	gui_about.o hash.o server.o serverIndex.o sha.o superBlock.o

LINKER_OPTIONS = `pkg-config gtkmm-2.4 --cflags --libs` \
	-lboost_thread -D_REENTRANT -lboost_filesystem 

main: $(OBJECTS)
	$(CCL) $(OBJECTS) -o ../simple-p2p $(LINKER_OPTIONS)
	@echo "\nBuilding simple-p2p completed!\n"

main.o: main.cc gui.o
	$(CC) main.cc `pkg-config gtkmm-2.4 --cflags`

client.o: client.cc client.h download.o clientIndex.o exploration.o globals.h 
	$(CC) client.cc

download.o: download.cc download.h hash.o superBlock.o globals.h
	$(CC) download.cc

clientIndex.o: clientIndex.cc clientIndex.h download.o exploration.o globals.h
	$(CC) clientIndex.cc

exploration.o: exploration.cc exploration.h globals.h
	$(CC) exploration.cc

gui.o: gui.cc gui.h client.o exploration.o gui_about.o server.o globals.h
	$(CC) gui.cc `pkg-config gtkmm-2.4 --cflags`

gui_about.o: gui_about.cc gui_about.h globals.h
	$(CC) gui_about.cc `pkg-config gtkmm-2.4 --cflags`

hash.o: hash.cc hash.h sha.o globals.h
	$(CC) hash.cc

server.o: server.cc server.h serverIndex.o globals.h
	$(CC) server.cc

serverIndex.o: serverIndex.cc serverIndex.h globals.h
	$(CC) serverIndex.cc

sha.o: sha.cc sha.h globals.h
	$(CC) sha.cc

superBlock.o: superBlock.cc superBlock.h globals.h
	$(CC) superBlock.cc

clean:
	rm -v *.o

