CXXFLAGS = `pkg-config gtkmm-2.4 --cflags` -O3 -Werror -ansi -pedantic
#GPROF = -pg

OBJECTS = \
	DB_blacklist.o \
	client.o \
	client_buffer.o \
	client_new_connection.o \
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
	speed_calculator.o

SERVER_LINKER_OPTIONS = \
	-lboost_filesystem-gcc-mt-1_33_1 \
	-lboost_thread-gcc-mt-1_33_1 \
	-lboost_filesystem \
	-lboost_thread \
	-D_REENTRANT \
	-lsqlite3

.depend:
	$(CXX) -MM *.cc > .depend

main: $(OBJECTS) server
	$(CXX) $(OBJECTS) $(GPROF) -o p2p $(LINKER_OPTIONS)
	@echo "building complete"

server: $(SERVER_OBJECTS)
	$(CXX) $(SERVER_OBJECTS) $(GPROF) -o p2p_server $(SERVER_LINKER_OPTIONS)

-include .depend

clean:
	rm -v p2p p2p_server .depend main_server.o $(OBJECTS)
