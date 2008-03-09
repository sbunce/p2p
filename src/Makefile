#build dependencies(debian package names)
#g++ libgtkmm-2.4-dev libboost-thread-dev libboost-filesystem-dev libsqlite3-dev

CC = g++ -c -O3
CCL = g++
#GPROF = -pg

#objects for the full program
OBJECTS = main.o client.o client_buffer.o conversion.o DB_access.o download.o \
	download_conn.o download_file.o download_file_conn.o download_hash_tree.o \
	gui.o gui_about.o hash_tree.o server.o server_index.o sha.o \
	speed_calculator.o

#objects for the command line server
SERVER_OBJECTS = main_server.o conversion.o DB_access.o hash_tree.o server.o \
	server_index.o sha.o speed_calculator.o

#linker options for the full program
LINKER_OPTIONS = `pkg-config gtkmm-2.4 --cflags --libs` -lboost_filesystem \
	-lboost_thread -D_REENTRANT -lsqlite3 

#linker options for the command line server
SERVER_LINKER_OPTIONS = -lboost_filesystem -lboost_thread -D_REENTRANT -lsqlite3 

main: $(OBJECTS)
	$(CCL) $(OBJECTS) $(GPROF) -o ../p2p $(LINKER_OPTIONS)
	@echo "Building p2p complete."

server: $(SERVER_OBJECTS)
	$(CCL) $(SERVER_OBJECTS) $(GPROF) -o ../p2p_server $(SERVER_LINKER_OPTIONS)
	@echo "Building p2p server complete."

main.o: main.cc gui.o
	$(CC) main.cc `pkg-config gtkmm-2.4 --cflags`

main_server.o: main_server.cc server.o
	$(CC) main_server.cc

client.o: client.cc client.h DB_access.o download.o download_conn.o \
	download_file.o download_file_conn.o client_buffer.o  speed_calculator.o \
	atomic.h thread_pool.h global.h
	$(CC) client.cc

client_buffer.o: client_buffer.cc client_buffer.h conversion.o download.o \
	atomic.h global.h
	$(CC) client_buffer.cc

conversion.o: conversion.cc conversion.h
	$(CC) conversion.cc

DB_access.o: DB_access.cc DB_access.h hash_tree.o global.h
	$(CC) DB_access.cc

download.o: download.cc download.h atomic.h download_conn.o speed_calculator.o \
	global.h
	$(CC) download.cc

download_conn.o: download_conn.cc download_conn.h
	$(CC) download_conn.cc

download_file.o: download_file.cc download_file.h conversion.o DB_access.o \
	download.o download_file_conn.o hash_tree.o atomic.h global.h
	$(CC) download_file.cc

download_file_conn.o: download_file_conn.cc download_file_conn.h \
	download_conn.o speed_calculator.o
	$(CC) download_file_conn.cc

download_hash_tree.o: download_hash_tree.cc download_hash_tree.h download.o \
	hash_tree.o
	$(CC) download_hash_tree.cc

gui.o: gui.cc gui.h client.o DB_access.o gui_about.o server.o global.h
	$(CC) gui.cc `pkg-config gtkmm-2.4 --cflags`

gui_about.o: gui_about.cc gui_about.h global.h
	$(CC) gui_about.cc `pkg-config gtkmm-2.4 --cflags`

hash_tree.o: hash_tree.cc hash_tree.h sha.o global.h
	$(CC) hash_tree.cc

server.o: server.cc server.h server_index.o speed_calculator.o conversion.o \
	atomic.h global.h
	$(CC) server.cc

server_index.o: server_index.cc server_index.h atomic.h DB_access.o global.h
	$(CC) server_index.cc

sha.o: sha.cc sha.h global.h
	$(CC) sha.cc

speed_calculator.o: speed_calculator.cc speed_calculator.h global.h
	$(CC) speed_calculator.cc

clean:
	rm -v *.o
