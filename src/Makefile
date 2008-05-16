#GPROF = -pg
CXXFLAGS = `pkg-config gtkmm-2.4 --cflags` -O3 #-Werror -ansi -pedantic

OBJECTS = \
	client_buffer.o \
	client.o \
	conversion.o \
	DB_access.o \
	download_conn.o \
	download_file_conn.o \
	download_file.o \
	download_hash_tree.o \
	download_prep.o \
	download.o \
	gui.o \
	gui_about.o \
	gui_preferences.o \
	hash_tree.o \
	logger.o \
	main.o \
	request_gen.o \
	server_index.o \
	server.o \
	sha.o \
	speed_calculator.o

LINKER_OPTIONS = \
	`pkg-config gtkmm-2.4 --libs` \
	-lboost_filesystem \
	-lboost_thread -D_REENTRANT \
	-lsqlite3

SERVER_OBJECTS = \
	conversion.o \
	DB_access.o \
	hash_tree.o \
	logger.o \
	main_server.o \
	server_index.o \
	server.o \
	sha.o \
	speed_calculator.o

SERVER_LINKER_OPTIONS = \
	-lboost_filesystem \
	-lboost_thread \
	-D_REENTRANT \
	-lsqlite3

.depend:
	fastdep --extraremakedep=Makefile --remakedeptarget=.depend *.cc > .depend

main: $(OBJECTS) server
	g++ $(OBJECTS) $(GPROF) -o p2p $(LINKER_OPTIONS)
	@echo "building complete"

server: $(SERVER_OBJECTS)
	g++ $(SERVER_OBJECTS) $(GPROF) -o p2p_server $(SERVER_LINKER_OPTIONS)

-include .depend

clean:
	rm -v p2p p2p_server .depend main_server.o $(OBJECTS)
