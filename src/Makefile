#GPROF = -pg
CXXFLAGS = `pkg-config gtkmm-2.4 --cflags` -O3 -Werror -ansi -pedantic

OBJECTS = \
	client_buffer.o \
	client.o \
	DB_client_preferences.o \
	DB_download.o \
	DB_search.o \
	DB_server_preferences.o \
	DB_share.o \
	download_conn.o \
	download_file_conn.o \
	download_file.o \
	download_hash_tree.o \
	download_hash_tree_conn.o \
	download_factory.o \
	download.o \
	gui.o \
	gui_about.o \
	gui_preferences.o \
	hash_tree.o \
	logger.o \
	main.o \
	request_gen.o \
	server.o \
	server_buffer.o \
	server_index.o \
	server_protocol.o \
	sha.o \
	speed_calculator.o

LINKER_OPTIONS = \
	`pkg-config gtkmm-2.4 --libs` \
	-lboost_filesystem \
	-lboost_thread \
	-D_REENTRANT \
	-lsqlite3

SERVER_OBJECTS = \
	DB_download.o \
	DB_server_preferences.o \
	DB_share.o \
	hash_tree.o \
	logger.o \
	main_server.o \
	server.o \
	server_buffer.o \
	server_index.o \
	server_protocol.o \
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
