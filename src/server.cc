#include "server.h"

server::server():
	connections(0),
	send_pending(0),
	stop_threads(false),
	threads(0)
{
	FD_ZERO(&master_FDS);
	Speed_Calculator.set_speed_limit(DB_Server_Preferences.get_speed_limit_uint());
	boost::thread T(boost::bind(&server::main_thread, this));
}

server::~server()
{
	stop_threads = true;
	raise(SIGINT); //force select() to return
	while(threads){
		usleep(1);
	}
}

void server::current_uploads(std::vector<upload_info> & info)
{
	std::map<int, server_buffer>::iterator iter_cur, iter_end;
	iter_cur = Server_Buffer.begin();
	iter_end = Server_Buffer.end();
	while(iter_cur != iter_end){
		iter_cur->second.current_uploads(info);
		++iter_cur;
	}
}

void server::disconnect(const int & socket_FD)
{
	logger::debug(LOGGER_P1,"disconnecting socket ",socket_FD);
	FD_CLR(socket_FD, &master_FDS);
	close(socket_FD);
	--connections;


	Server_Buffer.erase(socket_FD);

	//reduce FD_max if possible
	for(int x = FD_max; x != 0; --x){
		if(FD_ISSET(x, &master_FDS)){
			FD_max = x;
			break;
		}
	}
}

int server::get_max_connections()
{
	return DB_Server_Preferences.get_max_connections();
}

void server::set_max_connections(int max_connections)
{
	DB_Server_Preferences.set_max_connections(max_connections);
}

std::string server::get_share_directory()
{
	return DB_Server_Preferences.get_share_directory();
}

void server::set_share_directory(const std::string & share_directory)
{
	DB_Server_Preferences.set_share_directory(share_directory);
}

std::string server::get_speed_limit()
{
	std::ostringstream sl;
	sl << Speed_Calculator.get_speed_limit() / 1024;
	return sl.str();
}

void server::set_speed_limit(const std::string & speed_limit)
{
	std::stringstream ss(speed_limit);
	unsigned int speed;
	ss >> speed;
	speed *= 1024;
	DB_Server_Preferences.set_speed_limit(speed);
	Speed_Calculator.set_speed_limit(speed);
}

bool server::is_indexing()
{
	return Server_Index.is_indexing();
}

bool server::new_connection(const int & listener)
{
	sockaddr_in remoteaddr;
	socklen_t len = sizeof(remoteaddr);
	int new_FD = accept(listener, (sockaddr *)&remoteaddr, &len);
	if(new_FD == -1){
		perror("accept");
		return false;
	}

	std::string new_IP(inet_ntoa(remoteaddr.sin_addr));

	//make sure the client isn't already connected
	sockaddr_in temp_addr;
	for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
		if(FD_ISSET(socket_FD, &master_FDS)){
			getpeername(socket_FD, (sockaddr*)&temp_addr, &len);
			if(strcmp(new_IP.c_str(), inet_ntoa(temp_addr.sin_addr)) == 0){
				logger::debug(LOGGER_P1,"server ",new_IP," attempted multiple connections");
				close(new_FD);
				return false;
			}
		}
	}

	if(connections <= global::MAX_CONNECTIONS){
		++connections;
		Server_Buffer.insert(std::make_pair(new_FD, server_buffer(new_FD, new_IP)));
		FD_SET(new_FD, &master_FDS);
		if(new_FD > FD_max){
			FD_max = new_FD;
		}
		logger::debug(LOGGER_P1,"client ",inet_ntoa(remoteaddr.sin_addr)," socket ",new_FD," connected");
	}else{
		close(new_FD);
	}

	return true;
}

void server::main_thread()
{
	++threads;

	//set listening socket
	int listener;
	if((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(1);
	}

	/*
	Reuse port if it's currently in use. It can take up to a minute for linux to
	deallocate the port if this isn't used.
	*/
	int yes = 1;
	if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
		perror("setsockopt");
		exit(1);
	}

	sockaddr_in myaddr;                        //server address
	myaddr.sin_family = AF_INET;               //set ipv4
	myaddr.sin_addr.s_addr = INADDR_ANY;       //set to listen on all available interfaces
	myaddr.sin_port = htons(global::P2P_PORT); //set listening port
	memset(&(myaddr.sin_zero), '\0', 8);

	//set listener info to what we set above
	if(bind(listener, (sockaddr *)&myaddr, sizeof(myaddr)) == -1){
		perror("bind");
		exit(1);
	}

	//start listener socket listening
	if(listen(listener, 10) == -1){
		perror("listen");
		exit(1);
	}
	logger::debug(LOGGER_P1,"created listening socket ",listener);

	FD_SET(listener, &master_FDS);
	FD_max = listener;

	char recv_buff[global::S_MAX_SIZE*global::PIPELINE_SIZE];
   int n_bytes;
	int send_limit = 0;
	while(true){
		if(stop_threads){
			break;
		}

		/*
		This if(send_pending) exists to saves CPU time by not checking if sockets
		are ready to write when we don't need to write anything.
		*/
		if(send_pending != 0){
			read_FDS = master_FDS;
			write_FDS = master_FDS;
			if((select(FD_max+1, &read_FDS, &write_FDS, NULL, NULL)) == -1){
				//gprof will send PROF signal, this will ignore it
				if(errno != EINTR){
					perror("server select");
					exit(1);
				}
			}
		}else{
			FD_ZERO(&write_FDS);
			read_FDS = master_FDS;
			if((select(FD_max+1, &read_FDS, NULL, NULL, NULL)) == -1){
				//gprof will send PROF signal, this will ignore it
				if(errno != EINTR){
					perror("server select");
					exit(1);
				}
			}
		}

		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &read_FDS)){
				if(FD_ISSET(listener, &read_FDS)){
					//new client connected
					new_connection(listener);
				}else{
					//existing socket sending data
					if((n_bytes = recv(socket_FD, recv_buff, global::S_MAX_SIZE*global::PIPELINE_SIZE, 0)) <= 0){
						//error or disconnect
						disconnect(socket_FD);
						continue;
					}else{
						//incoming data from client socket
						std::map<int, server_buffer>::iterator iter = Server_Buffer.find(socket_FD);
						assert(iter != Server_Buffer.end());
						server_buffer * SB = &iter->second;
						process_request(SB, recv_buff, n_bytes);
					}
				}
			}

			//do not check for writes on listener
			if(FD_ISSET(socket_FD, &write_FDS) && socket_FD != listener){
				std::map<int, server_buffer>::iterator iter = Server_Buffer.find(socket_FD);
				assert(iter != Server_Buffer.end());
				server_buffer * SB = &iter->second;
				if(!SB->send_buff.empty()){
					send_limit = Speed_Calculator.rate_control(SB->send_buff.size());
					if((n_bytes = send(socket_FD, SB->send_buff.c_str(), send_limit, MSG_NOSIGNAL)) < 0){
						disconnect(socket_FD);
					}else{
						//remove bytes sent from buffer
						Speed_Calculator.update(n_bytes);
						SB->send_buff.erase(0, n_bytes);
						if(SB->send_buff.empty()){
							--send_pending;
						}
					}
				}
			}
		}
	}

	--threads;
}

void server::process_request(server_buffer * SB, char * recv_buff, const int & n_bytes)
{
	SB->recv_buff.append(recv_buff, n_bytes);

	//disconnect clients that have pipelined more than is allowed
	if(SB->recv_buff.size() > global::S_MAX_SIZE*global::PIPELINE_SIZE){
		logger::debug(LOGGER_P1,"disconnecting abusive socket ",SB->socket_FD);
		disconnect(SB->socket_FD);
//blacklist needs to be done here
	}

	bool initial_empty = (SB->send_buff.size() == 0);
	Server_Protocol.process(SB);
	if(initial_empty && SB->send_buff.size() != 0){
		//buffer went from empty to non-empty, signal that a send needs to be done
		++send_pending;
	}
}

int server::total_speed()
{
	Speed_Calculator.update(0);
	return Speed_Calculator.speed();
}
