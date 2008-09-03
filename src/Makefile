CXXFLAGS := -O3 -Werror -ansi -Isqlite3 -Ilibtommath

#subdirs to call submakes on
SUBDIRS = libtommath sqlite3

#targets that require gtkmm
main.o: CXXFLAGS += `pkg-config gtkmm-2.4 --cflags`
gui.o: CXXFLAGS += `pkg-config gtkmm-2.4 --cflags`
gui_about.o: CXXFLAGS += `pkg-config gtkmm-2.4 --cflags`
gui_preferences.o: CXXFLAGS += `pkg-config gtkmm-2.4 --cflags`

LIBTOMMATH_OBJECT := libtommath/libtommath.a
SQLITE3_OBJECT := sqlite3/sqlite3.o

OBJECTS := \
	CLI_args.o \
	client.o \
	client_buffer.o \
	client_new_connection.o \
	client_server_bridge.o \
	CMWC4096.o \
	DB_blacklist.o \
	DB_client_preferences.o \
	DB_download.o \
	DB_prime.o \
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
	gui_preferences.o \
	hash_tree.o \
	logger.o \
	main.o \
	number_generator.o \
	request_generator.o \
	server.o \
	server_buffer.o \
	server_index.o \
	sha.o \
	speed_calculator.o \
	thread_pool.o

SERVER_OBJECTS := \
	CLI_args.o \
	client_server_bridge.o \
	CMWC4096.o \
	DB_blacklist.o \
	DB_download.o \
	DB_prime.o \
	DB_server_preferences.o \
	DB_share.o \
	encryption.o \
	hash_tree.o \
	logger.o \
	main_server.o \
	number_generator.o \
	server.o \
	server_buffer.o \
	server_index.o \
	sha.o \
	speed_calculator.o

LINKER_OPTIONS := \
	`pkg-config gtkmm-2.4 --libs` \
	-ldl \
	-lboost_filesystem \
	-lboost_thread \
	-D_REENTRANT

SERVER_LINKER_OPTIONS := \
	-ldl \
	-lboost_filesystem \
	-lboost_thread

.PHONY: all
all: $(OBJECTS) $(SERVER_OBJECTS) subdirs
	$(CXX) -o p2p $(OBJECTS) $(LIBTOMMATH_OBJECT) $(SQLITE3_OBJECT) $(LINKER_OPTIONS)
	$(CXX) -o p2p_server $(SERVER_OBJECTS) $(LIBTOMMATH_OBJECT) $(SQLITE3_OBJECT) $(SERVER_LINKER_OPTIONS)
	@echo building complete

#print how many lines of code exist
.PHONY: loc
loc:
	@echo lines of code: $(shell cat *.h *.cc Makefile |wc -l)

.depend:
	$(CXX) -MM *.cc > .depend

-include .depend

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: clean
clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f p2p p2p_server .depend *.o
