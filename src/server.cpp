#include "server.hpp"

server::server()
: connections(0)
{
	FD_ZERO(&master_FDS);
	max_connections = DB_Preferences.get_server_connections();
	rate_limit::set_upload_rate(DB_Preferences.get_upload_rate());

	#ifdef WIN32
	//start winsock
	WORD wsock_ver = MAKEWORD(1,1);
	WSADATA wsock_data;
	int startup;
	if((startup = WSAStartup(wsock_ver, &wsock_data)) != 0){
		LOGGER << "winsock startup error " << startup;
		exit(1);
	}
	#endif

	main_thread = boost::thread(boost::bind(&server::main_loop, this));
}

server::~server()
{
	main_thread.interrupt();
	raise(SIGINT); //force select() to return
	main_thread.join();

	#ifdef WIN32
	WSACleanup();
	#endif
}

void server::check_blacklist(int & blacklist_state)
{
	if(DB_Blacklist.modified(blacklist_state)){
		sockaddr_in temp_addr;
		socklen_t len = sizeof(temp_addr);
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &master_FDS)){
				getpeername(socket_FD, (sockaddr*)&temp_addr, &len);
				std::string IP(inet_ntoa(temp_addr.sin_addr));
				if(DB_Blacklist.is_blacklisted(IP)){
					LOGGER << "disconnecting blacklisted IP " << IP;
					disconnect(socket_FD);
				}
			}
		}
	}
}

void server::current_uploads(std::vector<upload_info> & info)
{
	server_buffer::current_uploads(info);
}

void server::disconnect(const int & socket_FD)
{
	LOGGER << "disconnecting socket " << socket_FD;
	FD_CLR(socket_FD, &master_FDS);
	--connections;

	#ifdef WIN32
	closesocket(socket_FD);
	#else
	close(socket_FD);
	#endif

	server_buffer::erase(socket_FD);

	//reduce FD_max if possible
	for(int x = FD_max; x != 0; --x){
		if(FD_ISSET(x, &master_FDS)){
			FD_max = x;
			break;
		}
	}
}

unsigned server::get_max_connections()
{
	return max_connections;
}

std::string server::get_share_directory()
{
	return DB_Preferences.get_share_directory();
}

unsigned server::get_upload_rate()
{
	if(rate_limit::get_upload_rate() == std::numeric_limits<unsigned>::max()){
		return 0;
	}else{
		return rate_limit::get_upload_rate();
	}
}

bool server::is_indexing()
{
	return server_index::is_indexing();
}

void server::new_connection(const int & listener)
{
	sockaddr_in remoteaddr;
	socklen_t len = sizeof(remoteaddr);
	int new_FD = accept(listener, (sockaddr *)&remoteaddr, &len);
	if(new_FD == -1){
		#ifdef WIN32
		LOGGER << "winsock error " << WSAGetLastError();
		#else
		perror("accept");
		#endif
		return;
	}

	std::string new_IP(inet_ntoa(remoteaddr.sin_addr));

	//do not allow clients to connect that are blacklisted
	if(DB_Blacklist.is_blacklisted(new_IP)){
		LOGGER << "stopping connection from blacklisted IP " << new_IP;
		return;
	}

	#ifndef ALLOW_LOCALHOST_CONNECTION
	if(new_IP.find("127.") != std::string::npos){
		LOGGER << "stopping connection to localhost";
		return;
	}
	#endif

	//make sure the client isn't already connected
	sockaddr_in temp_addr;
	for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
		if(FD_ISSET(socket_FD, &master_FDS)){
			getpeername(socket_FD, (sockaddr*)&temp_addr, &len);
			if(strcmp(new_IP.c_str(), inet_ntoa(temp_addr.sin_addr)) == 0){
				LOGGER << "server " << new_IP << " attempted multiple connections";
				#ifdef WIN32
				closesocket(new_FD);
				#else
				close(new_FD);
				#endif
				return;
			}
		}
	}

	if(connections < max_connections){
		++connections;
		server_buffer::new_connection(new_FD, new_IP);
		FD_SET(new_FD, &master_FDS);
		if(new_FD > FD_max){
			FD_max = new_FD;
		}
		LOGGER << "client " << inet_ntoa(remoteaddr.sin_addr) << " socket " << new_FD << " connected";
	}else{
		#ifdef WIN32
		closesocket(new_FD);
		#else
		close(new_FD);
		#endif
	}

	return;
}

void server::main_loop()
{
	//give GUI some time to start
	portable_sleep::ms(2000);

	//start server index if not already started
	server_index::init();

	//set listening socket
	unsigned listener;
	if((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		#ifdef WIN32
		LOGGER << "winsock error " << WSAGetLastError();
		#else
		perror("socket");
		#endif
		exit(1);
	}

	/*
	Reuse port if it's currently in use. It can take up to a minute for linux to
	deallocate the port if this isn't used.
	*/
	int yes = 1;
	#ifdef WIN32
	if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(int)) == -1){
	#else
	if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
	#endif
		#ifdef WIN32
		LOGGER << "winsock error " << WSAGetLastError();
		#else
		perror("setsockopt");
		#endif
		exit(1);
	}

	sockaddr_in myaddr;                        //server address
	myaddr.sin_family = AF_INET;               //set ipv4
	myaddr.sin_addr.s_addr = INADDR_ANY;       //set to listen on all available interfaces
	myaddr.sin_port = htons(global::P2P_PORT); //set listening port
	memset(&(myaddr.sin_zero), '\0', 8);

	//set listener info to what we set above
	if(bind(listener, (sockaddr *)&myaddr, sizeof(myaddr)) == -1){
		#ifdef WIN32
		LOGGER << "winsock error " << WSAGetLastError();
		#else
		perror("bind");
		#endif
		exit(1);
	}

	//start listener socket listening
	if(listen(listener, 10) == -1){
		#ifdef WIN32
		LOGGER << "winsock error " << WSAGetLastError();
		#else
		perror("listen");
		#endif
		exit(1);
	}
	LOGGER << "created listening socket " << listener;

	FD_SET(listener, &master_FDS);
	FD_max = listener;

	//see documentation in database_table_blacklist to see what this does
	int blacklist_state = 0;

	/*
	Holds a copy of a chunk of the send buffer within the server_buffer.
	This buffer is needed so that the client_buffer can be fully thread
	safe. If we were reading from the send_buff within the client_buffer
	and someone called server_buffer::erase on it then we'd have trouble.
	*/
	std::string send_buff;
	int max_send_buff = global::C_MAX_SIZE * global::PIPELINE_SIZE;
	send_buff.reserve(max_send_buff);

	char recv_buff[global::S_MAX_SIZE*global::PIPELINE_SIZE];
	int n_bytes;             //how many bytes sent or received by send()/recv()
	unsigned transfer_limit; //speed_calculator stores how much may be sent here
	bool transfer;           //triggers sleep if nothing sent or received last iteration
	unsigned max_send = 0;   //see documentation right above socket checking loop

	while(true){
		boost::this_thread::interruption_point();
		check_blacklist(blacklist_state);

		/*
		This if(send_pending) exists to saves CPU time by not checking if sockets
		are ready to write when we don't need to write anything.
		*/
		if(server_buffer::get_send_pending() != 0){
			read_FDS = master_FDS;
			write_FDS = master_FDS;
			if(select(FD_max+1, &read_FDS, &write_FDS, NULL, NULL) == -1){
				//gprof will send PROF signal, this will ignore it
				if(errno != EINTR){
					#ifdef WIN32
					LOGGER << "winsock error " << WSAGetLastError();
					#else
					perror("server select");
					#endif
					exit(1);
				}
			}
		}else{
			FD_ZERO(&write_FDS);
			read_FDS = master_FDS;
			if(select(FD_max+1, &read_FDS, NULL, NULL, NULL) == -1){
				//gprof will send PROF signal, this will ignore it
				if(errno != EINTR){
					#ifdef WIN32
					LOGGER << "winsock error " << WSAGetLastError();
					#else
					perror("server select");
					#endif
					exit(1);
				}
			}
		}

		/*
		There is a starvation problem that happens when rate limiting. If all sockets
		are checked in order and the rate limit is lower than the total bandwidth
		then the higher socket numbers will get starved. To compensate for this
		we set a max_read = rate_limit / connections. This way, in the most demanding
		scenario each sockets gets to be read at least once. A lower limit on
		max_read is set at 1 of course.
		*/
		if(connections == 0){
			max_send = 1;
		}else{
			/*
			rate_limit::get_upload_rate() returns unsigned int, the send() function
			take a signed int. If max_recv is beyond the max size of an int then
			the conversion can yield a negative number.

			This is fixed here because it's nice to have an unsigned everywhere
			else because rates cannot logically be negative.
			*/
			unsigned tmp;
			if((tmp = rate_limit::get_download_rate() / connections) > std::numeric_limits<int>::max()){
				max_send = std::numeric_limits<int>::max();
			}else{
				max_send = tmp;
			}
		}

		transfer = false;
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &read_FDS)){
				if(socket_FD == listener){
					new_connection(listener);
				}else{
					if((n_bytes = recv(socket_FD, recv_buff, global::S_MAX_SIZE*global::PIPELINE_SIZE, 0)) <= 0){
						disconnect(socket_FD);
						continue;
					}else{
						rate_limit::add_download_bytes(n_bytes);
						server_buffer::post_recv(socket_FD, recv_buff, n_bytes);
						transfer = true;
					}
				}
			}

			if(socket_FD != listener && FD_ISSET(socket_FD, &write_FDS)){
				if(server_buffer::get_send_buff(socket_FD, max_send_buff, send_buff)){
					if((transfer_limit = rate_limit::upload_rate_control(send_buff.size())) != 0){
						if(transfer_limit > max_send){
							transfer_limit = max_send;
						}
						if((n_bytes = send(socket_FD, send_buff.data(), send_buff.size(), MSG_NOSIGNAL)) < 0){
							disconnect(socket_FD);
						}else{
							rate_limit::add_upload_bytes(n_bytes);
							if(!server_buffer::post_send(socket_FD, n_bytes)){
								//disconnect upon empty buffer (server buffer requested this)
								disconnect(socket_FD);
							}
							transfer = true;
						}
					}
				}
			}
		}

		if(!transfer){
			portable_sleep::ms(1);
		}
	}
}

void server::set_max_connections(const unsigned & max_connections_in)
{
	max_connections = max_connections_in;
	DB_Preferences.set_server_connections(max_connections);
	while(connections > max_connections){
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &master_FDS)){
				disconnect(socket_FD);
				break;
			}
		}
	}
}

void server::set_share_directory(const std::string & share_directory)
{
	DB_Preferences.set_share_directory(share_directory);
	std::cout << "SETTING SHARE FEATURE UNIMPLEMENTED\n";
}

void server::set_max_upload_rate(unsigned upload_rate)
{
	if(upload_rate == 0){
		upload_rate = std::numeric_limits<unsigned>::max();
	}
	DB_Preferences.set_upload_rate(upload_rate);
	rate_limit::set_upload_rate(upload_rate);
}

unsigned server::total_rate()
{
	return rate_limit::upload_speed();
}
