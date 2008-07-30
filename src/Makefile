#tagets which require gtk
gui.o: CXXFLAGS += `pkg-config gtkmm-2.4 --cflags`
gui_preferences.o: CXXFLAGS += `pkg-config gtkmm-2.4 --cflags`
gui_about.o: CXXFLAGS += `pkg-config gtkmm-2.4 --cflags`
main.o: CXXFLAGS += `pkg-config gtkmm-2.4 --cflags`

OBJECTS = \
	client.o \
	client_buffer.o \
	client_new_connection.o \
	DB_blacklist.o \
	DB_client_preferences.o \
	DB_download.o \
	DB_search.o \
	DB_server_preferences.o \
	DB_share.o \
	download.o \
	download_factory.o \
	download_file.o \
	download_hash_tree.o \
	download_tracker.o \
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
	speed_calculator.o \
	sqlite3.o \
	thread_pool.o

LINKER_OPTIONS = \
	`pkg-config gtkmm-2.4 --libs` \
	-lboost_filesystem \
	-lboost_thread \
	-D_REENTRANT \
	-lsqlite3

SERVER_OBJECTS = \
	DB_blacklist.o \
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
	speed_calculator.o \
	sqlite3.o

all: $(OBJECTS) server
	$(CXX) $(OBJECTS) $(GPROF) -o p2p $(LINKER_OPTIONS)

server: $(SERVER_OBJECTS)
	$(CXX) $(SERVER_OBJECTS) $(GPROF) -o p2p_server $(LINKER_OPTIONS)

.depend:
	$(CXX) -MM *.cc > .depend

-include .depend

sqlite3.o: sqlite3.c sqlite3.h sqlite3ext.h

.PHONY: clean
clean:
	rm -v p2p p2p_server .depend main_server.o $(OBJECTS)
