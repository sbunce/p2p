CXXFLAGS += -Isqlite3 -Ibignum `pkg-config gtkmm-2.4 --cflags`

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
	encryption.o \
	gui.o \
	gui_about.o \
	gui_download_info.o \
	gui_preferences.o \
	hash_tree.o \
	logger.o \
	main.o \
	mersenne_twister.o \
	request_gen.o \
	server.o \
	server_buffer.o \
	server_index.o \
	server_protocol.o \
	sha.o \
	speed_calculator.o \
	sqlite3/sqlite3.o \
	thread_pool.o

LINKER_OPTIONS = \
	`pkg-config gtkmm-2.4 --libs` \
	-ldl \
	-lboost_filesystem \
	-lboost_thread \
	-D_REENTRANT

SERVER_OBJECTS = \
	DB_blacklist.o \
	DB_download.o \
	DB_server_preferences.o \
	DB_share.o \
	encryption.o \
	hash_tree.o \
	logger.o \
	main_server.o \
	mersenne_twister.o \
	server.o \
	server_buffer.o \
	server_index.o \
	server_protocol.o \
	sha.o \
	speed_calculator.o \
	sqlite3/sqlite3.o

SERVER_LINKER_OPTIONS = \
	-ldl \
	-lboost_filesystem \
	-lboost_thread

all: $(OBJECTS) bignum sqlite3 server
	$(CXX) $(OBJECTS) $(GPROF) -o p2p $(LINKER_OPTIONS)

server: $(SERVER_OBJECTS) bignum sqlite3
	$(CXX) $(SERVER_OBJECTS) $(GPROF) -o p2p_server $(SERVER_LINKER_OPTIONS)

#automatic dependency generation
.depend:
	$(CXX) -MM *.cc > .depend

-include .depend

.PHONY: bignum
bignum:
	$(MAKE) -C bignum

.PHONY: sqlite3
sqlite3:
	$(MAKE) -C sqlite3

.PHONY: clean
clean:
	$(MAKE) -C bignum clean
	$(MAKE) -C sqlite3 clean
	rm -f p2p p2p_server .depend main_server.o $(OBJECTS)
