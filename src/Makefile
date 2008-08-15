CXXFLAGS = $(GPROF) -O3 -Werror -ansi -Ilibtommath -Isqlite3 `pkg-config gtkmm-2.4 --cflags`
CFLAGS = -O3 -Werror -ansi

LIBTOMMATH_OBJECT = libtommath/libtommath.a
SQLITE3_OBJECT = sqlite3/sqlite3.o

OBJECTS = \
	client.o \
	client_buffer.o \
	client_new_connection.o \
	CMWC4096.o \
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
	request_gen.o \
	server.o \
	server_buffer.o \
	server_index.o \
	sha.o \
	speed_calculator.o \
	thread_pool.o

LINKER_OPTIONS = \
	`pkg-config gtkmm-2.4 --libs` \
	-ldl \
	-lboost_filesystem \
	-lboost_thread \
	-D_REENTRANT

SERVER_OBJECTS = \
	CMWC4096.o \
	DB_blacklist.o \
	DB_download.o \
	DB_server_preferences.o \
	DB_share.o \
	encryption.o \
	hash_tree.o \
	logger.o \
	main_server.o \
	server.o \
	server_buffer.o \
	server_index.o \
	sha.o \
	speed_calculator.o

SERVER_LINKER_OPTIONS = \
	-ldl \
	-lboost_filesystem \
	-lboost_thread

all: $(OBJECTS) libtommath sqlite3 server
	$(CXX) $(OBJECTS) $(LIBTOMMATH_OBJECT) $(SQLITE3_OBJECT) -o p2p $(LINKER_OPTIONS) $(GPROF)

server: $(SERVER_OBJECTS) libtommath sqlite3
	$(CXX) $(SERVER_OBJECTS) $(LIBTOMMATH_OBJECT) $(SQLITE3_OBJECT) -o p2p_server $(SERVER_LINKER_OPTIONS) $(GPROF)

#automatic dependency generation
.depend:
	$(CXX) -MM *.cc > .depend

-include .depend

.PHONY: libtommath
libtommath:
	$(MAKE) --quiet -C libtommath

.PHONY: sqlite3
sqlite3:
	$(MAKE) -C sqlite3

.PHONY: clean
clean:
	$(MAKE) -C libtommath clean
	$(MAKE) -C sqlite3 clean
	rm -f p2p p2p_server .depend $(OBJECTS) $(SERVER_OBJECTS)
