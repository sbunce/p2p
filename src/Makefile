#build dependencies(ubuntu package names)
#g++ libboost-thread-dev libboost-filesystem-dev libsqlite3-dev

CC = g++ -O3 -c
CCL = g++

#GPROF = -pg

OBJECTS = main.o client.o client_buffer.o download.o download_file.o client_index.o \
	exploration.o gui.o gui_about.o hash.o server.o server_index.o sha.o \
	super_block.o

LINKER_OPTIONS = `pkg-config gtkmm-2.4 --cflags --libs` -lboost_filesystem \
	-lboost_thread -D_REENTRANT -lsqlite3 

main: $(OBJECTS)
	$(CCL) $(OBJECTS) $(GPROF) -o ../simple-p2p $(LINKER_OPTIONS)
	@echo "\nBuilding simple-p2p complete.\n"

main.o: main.cc gui.o
	$(CC) main.cc `pkg-config gtkmm-2.4 --cflags`

client.o: client.cc client.h download.o download_file.o client_buffer.o client_index.o exploration.o atomic.h global.h
	$(CC) client.cc

client_buffer.o: client_buffer.cc client_buffer.h download.o atomic.h conversion.h global.h
	$(CC) client_buffer.cc

client_index.o: client_index.cc client_index.h download.o exploration.o global.h
	$(CC) client_index.cc

download.o: download.cc download.h atomic.h global.h
	$(CC) download.cc

download_file.o: download_file.cc download_file.h download.o hash.o super_block.o atomic.h conversion.h global.h
	$(CC) download_file.cc

exploration.o: exploration.cc exploration.h global.h
	$(CC) exploration.cc

gui.o: gui.cc gui.h client.o exploration.o gui_about.o server.o global.h
	$(CC) gui.cc `pkg-config gtkmm-2.4 --cflags`

gui_about.o: gui_about.cc gui_about.h global.h
	$(CC) gui_about.cc `pkg-config gtkmm-2.4 --cflags`

hash.o: hash.cc hash.h sha.o global.h
	$(CC) hash.cc

server.o: server.cc server.h server_index.o conversion.h global.h
	$(CC) server.cc

server_index.o: server_index.cc server_index.h global.h
	$(CC) server_index.cc

sha.o: sha.cc sha.h global.h
	$(CC) sha.cc

super_block.o: super_block.cc super_block.h global.h
	$(CC) super_block.cc

clean:
	rm -v *.o

