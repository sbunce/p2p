#build dependencies(debian package names)
#g++ libgtkmm-2.4-dev libboost-thread-dev libboost-filesystem-dev libsqlite3-dev

CC = g++ -c -O3
CCL = g++
#GPROF = -pg

#objects for full program
OBJECTS = client_buffer.o client.o conversion.o DB_access.o download_conn.o \
	download_file_conn.o download_file.o download_hash_tree.o download.o \
	gui_about.o gui.o hash_tree.o main.o server_index.o server.o sha.o \
	speed_calculator.o

#linker options for full program
LINKER_OPTIONS = `pkg-config gtkmm-2.4 --cflags --libs` -lboost_filesystem \
	-lboost_thread -D_REENTRANT -lsqlite3 

#objects for command line server
SERVER_OBJECTS = conversion.o DB_access.o hash_tree.o main_server.o \
	server_index.o server.o sha.o speed_calculator.o

#linker options for command line server
SERVER_LINKER_OPTIONS = -lboost_filesystem -lboost_thread -D_REENTRANT -lsqlite3

#all objects depend on these
GLOBAL = global.h Makefile

main: $(OBJECTS) server
	$(CCL) $(OBJECTS) $(GPROF) -o p2p $(LINKER_OPTIONS)
	@echo "building p2p complete"

server: $(SERVER_OBJECTS)
	$(CCL) $(SERVER_OBJECTS) $(GPROF) -o p2p_server $(SERVER_LINKER_OPTIONS)

main.o: main.cc gui.h $(GLOBAL)
	$(CC) main.cc `pkg-config gtkmm-2.4 --cflags`

main_server.o: main_server.cc server.h $(GLOBAL)
	$(CC) main_server.cc

client.o: client.cc client.h client_buffer.h DB_access.h download_conn.h \
	download_file_conn.h download_file.h download.h speed_calculator.h \
	thread_pool.h $(GLOBAL)
	$(CC) client.cc

client_buffer.o: client_buffer.cc client_buffer.h atomic.h conversion.h \
	download.h $(GLOBAL)
	$(CC) client_buffer.cc

conversion.o: conversion.cc conversion.h $(GLOBAL)
	$(CC) conversion.cc

DB_access.o: DB_access.cc DB_access.h hash_tree.h $(GLOBAL)
	$(CC) DB_access.cc

download.o: download.cc download.h atomic.h download_conn.h speed_calculator.h \
	$(GLOBAL)
	$(CC) download.cc

download_conn.o: download_conn.cc download_conn.h $(GLOBAL)
	$(CC) download_conn.cc

download_file.o: download_file.cc download_file.h atomic.h conversion.h \
	DB_access.h download_file_conn.h download.h hash_tree.h $(GLOBAL)
	$(CC) download_file.cc

download_file_conn.o: download_file_conn.cc download_file_conn.h \
	download_conn.h speed_calculator.h $(GLOBAL)
	$(CC) download_file_conn.cc

download_hash_tree.o: download_hash_tree.cc download_hash_tree.h download.h \
	hash_tree.h $(GLOBAL)
	$(CC) download_hash_tree.cc

gui.o: gui.cc gui.h client.h DB_access.h gui_about.h server.h $(GLOBAL)
	$(CC) gui.cc `pkg-config gtkmm-2.4 --cflags`

gui_about.o: gui_about.cc gui_about.h $(GLOBAL)
	$(CC) gui_about.cc `pkg-config gtkmm-2.4 --cflags`

hash_tree.o: hash_tree.cc hash_tree.h sha.h $(GLOBAL)
	$(CC) hash_tree.cc

server.o: server.cc server.h atomic.h conversion.h server_index.h \
	speed_calculator.h $(GLOBAL)
	$(CC) server.cc

server_index.o: server_index.cc server_index.h atomic.h DB_access.h $(GLOBAL)
	$(CC) server_index.cc

sha.o: sha.cc sha.h $(GLOBAL)
	$(CC) sha.cc

speed_calculator.o: speed_calculator.cc speed_calculator.h $(GLOBAL)
	$(CC) speed_calculator.cc

clean:
	rm -v *.o
