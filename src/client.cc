#include "client.h"

client::client():
	blacklist_state(0),
	connections(0),
	FD_max(0),
	Download_Factory(),
	stop_threads(false),
	threads(0),
	Client_New_Connection(master_FDS, FD_max, max_connections, connections)
{
	boost::filesystem::create_directory(global::DOWNLOAD_DIRECTORY);
	max_connections = DB_Client_Preferences.get_max_connections();
	FD_ZERO(&master_FDS);
	Speed_Calculator.set_speed_limit(DB_Client_Preferences.get_speed_limit_uint());

	#ifdef WIN32
	//start winsock
	WORD wsock_ver = MAKEWORD(1,1);
	WSADATA wsock_data;
	int startup;
	if((startup = WSAStartup(wsock_ver, &wsock_data)) != 0){
		logger::debug(LOGGER_P1,"winsock startup error ", startup);
		exit(1);
	}
	#endif

	boost::thread T(boost::bind(&client::main_thread, this));

	//get number_generator singleton initialized so it starts generating primes
	number_generator::init();
}

client::~client()
{
	stop_threads = true;
	while(threads){
		portable_sleep::yield();
	}
	client_buffer::destroy();

	#ifdef WIN32
	//cleanup winsock
	WSACleanup();
	#endif
}

void client::check_blacklist()
{
	if(DB_blacklist::modified(blacklist_state)){
		sockaddr_in temp_addr;
		socklen_t len = sizeof(temp_addr);
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &master_FDS)){
				getpeername(socket_FD, (sockaddr*)&temp_addr, &len);
				std::string IP(inet_ntoa(temp_addr.sin_addr));
				if(DB_blacklist::is_blacklisted(IP)){
					logger::debug(LOGGER_P1,"disconnecting blacklisted IP ",IP);
					disconnect(socket_FD);
				}
			}
		}
	}
}

void client::check_timeouts()
{
	std::vector<int> timed_out;
	client_buffer::find_timed_out(timed_out);
	std::vector<int>::iterator iter_cur, iter_end;
	iter_cur = timed_out.begin();
	iter_end = timed_out.end();
	while(iter_cur != iter_end){
		disconnect(*iter_cur);
		++iter_cur;
	}
}

void client::current_downloads(std::vector<download_info> & info, std::string hash)
{
	client_buffer::current_downloads(info, hash);
}

inline void client::disconnect(const int & socket_FD)
{
	logger::debug(LOGGER_P1,"disconnecting socket ",socket_FD);

	#ifdef WIN32
	closesocket(socket_FD);
	#else
	close(socket_FD);
	#endif

	--connections;
	FD_CLR(socket_FD, &master_FDS);
	FD_CLR(socket_FD, &read_FDS);
	FD_CLR(socket_FD, &write_FDS);
	client_buffer::erase(socket_FD);

	//reduce FD_max if possible
	for(int x = FD_max; x != 0; --x){
		if(FD_ISSET(x, &master_FDS)){
			FD_max = x;
			break;
		}
	}
}

bool client::file_info(const std::string & hash, std::string & name, boost::uint64_t & tree_size, boost::uint64_t & file_size)
{
	return DB_Download.lookup_hash(hash, name, tree_size, file_size);
}

int client::prime_count()
{
	return number_generator::prime_count();
}

int client::get_max_connections()
{
	return max_connections;
}

void client::set_max_connections(int max_connections_in)
{
	max_connections = max_connections_in;
	DB_Client_Preferences.set_max_connections(max_connections);
	while(connections > max_connections){
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &master_FDS)){
				disconnect(socket_FD);
				break;
			}
		}
	}
}

std::string client::get_download_directory()
{
	return DB_Client_Preferences.get_download_directory();
}

void client::set_download_directory(const std::string & download_directory)
{
	DB_Client_Preferences.set_download_directory(download_directory);
}

std::string client::get_speed_limit()
{
	std::ostringstream sl;
	if(Speed_Calculator.get_speed_limit() == std::numeric_limits<unsigned int>::max()){
		sl << "0";
	}else{
		sl << Speed_Calculator.get_speed_limit() / 1024;
	}
	return sl.str();
}

void client::set_speed_limit(const std::string & speed_limit)
{
	std::stringstream ss(speed_limit);
	unsigned int speed;
	ss >> speed;
	speed *= 1024;
	if(speed == 0){
		speed = speed - 1; //max speed
	}
	DB_Client_Preferences.set_speed_limit(speed);
	Speed_Calculator.set_speed_limit(speed);
}

void client::main_thread()
{
	++threads;
	reconnect_unfinished();
	while(true){

		if(stop_threads){
			break;
		}

		check_blacklist();                  //check/disconnect blacklisted IPs
		check_timeouts();                   //check/disconnect timed out sockets
		client_buffer::generate_requests(); //process client_buffers, let them generate requests
		remove_complete();                  //remove completed downloads
		remove_empty();                     //remove empty client_buffers (no downloads)
		start_pending_downloads();          //start downloads queue'd by the GUI

		/*
		These must be initialized every iteration on linux(and possibly other OS's)
		because linux will change them(POSIX.1-2001 allows this) to reflect the
		time that select() has blocked for.
		*/
		timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		#ifdef WIN32
		//winsock doesn't allow calling select() with empty socket set
		if(master_FDS.fd_count == 0){
			portable_sleep::ms(1000);
			continue;
		}
		#endif

		if(client_buffer::get_send_pending() != 0){
			read_FDS = master_FDS;
			write_FDS = master_FDS;
			if(select(FD_max+1, &read_FDS, &write_FDS, NULL, &tv) == -1){
				if(errno != EINTR){ //EINTR is caused by gprof
					#ifdef WIN32
					logger::debug(LOGGER_P1,"winsock error ",WSAGetLastError());
					#else
					perror("client select");
					#endif
					exit(1);
				}
			}
		}else{
			FD_ZERO(&write_FDS);
			read_FDS = master_FDS;
			if(select(FD_max+1, &read_FDS, NULL, NULL, &tv) == -1){
				if(errno != EINTR){ //EINTR is caused by gprof
					#ifdef WIN32
					logger::debug(LOGGER_P1,"winsock error ",WSAGetLastError());
					#else
					perror("client select");
					#endif
					exit(1);
				}
			}
		}

		//process reads/writes
		char recv_buff[global::C_MAX_SIZE*global::PIPELINE_SIZE];
		int n_bytes;
		int recv_limit = 0;
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &read_FDS)){
				if((recv_limit = Speed_Calculator.rate_control(global::C_MAX_SIZE*global::PIPELINE_SIZE)) != 0){
					if((n_bytes = recv(socket_FD, recv_buff, recv_limit, MSG_NOSIGNAL)) <= 0){
						if(n_bytes == -1){
							#ifdef WIN32
							logger::debug(LOGGER_P1,"winsock error ",WSAGetLastError());
							#else
							perror("client recv");
							#endif
						}
						disconnect(socket_FD);
					}else{
						Speed_Calculator.update(n_bytes);
						client_buffer::recv_buff_append(socket_FD, recv_buff, n_bytes);
					}
				}
			}
			if(FD_ISSET(socket_FD, &write_FDS)){
				std::string * buff = &client_buffer::get_send_buff(socket_FD);
				if(!buff->empty()){
					if((n_bytes = send(socket_FD, buff->c_str(), buff->size(), MSG_NOSIGNAL)) < 0){
						if(n_bytes == -1){
							#ifdef WIN32
							logger::debug(LOGGER_P1,"winsock error ",WSAGetLastError());
							#elseremove_empty()
							perror("client send");
							#endif
						}
					}else{
						//remove bytes sent from buffer
						buff->erase(0, n_bytes);
						client_buffer::post_send(socket_FD);
					}
				}
			}
		}
	}
	--threads;
}

void client::reconnect_unfinished()
{
	std::vector<download_info> resume;
	DB_Download.resume(resume);
	std::vector<download_info>::iterator iter_cur, iter_end;
	iter_cur = resume.begin();
	iter_end = resume.end();
	while(iter_cur != iter_end){
		start_download(*iter_cur);
		++iter_cur;
	}
}

void client::remove_complete()
{
	std::list<download *> complete;
	client_buffer::find_complete(complete);
	if(complete.empty()){
		return;
	}

	std::list<download *>::iterator C_iter_cur, C_iter_end;
	C_iter_cur = complete.begin();
	C_iter_end = complete.end();
	while(C_iter_cur != C_iter_end){
		transition_download(*C_iter_cur);
		++C_iter_cur;
	}
}

void client::remove_empty()
{
	//disconnect any empty client_buffers
	std::vector<int> disconnect_sockets;
	client_buffer::find_empty(disconnect_sockets);
	std::vector<int>::iterator iter_cur, iter_end;
	iter_cur = disconnect_sockets.begin();
	iter_end = disconnect_sockets.end();
	while(iter_cur != iter_end){
		disconnect(*iter_cur);
		++iter_cur;
	}
}

void client::search(std::string search_word, std::vector<download_info> & Search_Info)
{
	DB_Search.search(search_word, Search_Info);
}

void client::start_download(const download_info & info)
{
	boost::mutex::scoped_lock lock(PD_mutex);
	Pending_Download.push_back(info);
}

void client::stop_download(std::string hash)
{
	client_buffer::stop_download(hash);
}

void client::start_download_process(const download_info & info)
{
	download * Download;
	std::list<download_connection> servers;
	if(Download_Factory.start_hash(info, Download, servers)){
		client_buffer::add_download(Download);
		std::list<download_connection>::iterator iter_cur, iter_end;
		iter_cur = servers.begin();
		iter_end = servers.end();
		while(iter_cur != iter_end){
			Client_New_Connection.queue(*iter_cur);
			++iter_cur;
		}
	}
}

void client::start_pending_downloads()
{
	/*
	The pending downloads are copied so that PD_mutex can be unlocked as soon as
	possible. PD_mutex locks start_download() which the GUI accesses. The
	start_download function needs to be as fast as possible so the GUI doesn't
	freeze up when starting a download.
	*/
	std::list<download_info> Pending_Download_Temp;
	{//begin lock scope
	boost::mutex::scoped_lock lock(PD_mutex);
	Pending_Download_Temp.assign(Pending_Download.begin(), Pending_Download.end());
	Pending_Download.clear();
	}//end lock scope

	std::list<download_info>::iterator iter_cur, iter_end;
	iter_cur = Pending_Download_Temp.begin();
	iter_end = Pending_Download_Temp.end();
	while(iter_cur != iter_end){
		//this is the slow function which neccessitates the copy
		start_download_process(*iter_cur);
		++iter_cur;
	}
}

int client::total_speed()
{
	/*
	If there is no I/O the speed won't get updated. This will update the speed
	as long as the client cares to check it.
	*/
	Speed_Calculator.update(0);
	return Speed_Calculator.speed();
}

void client::transition_download(download * Download_Stop)
{
	client_buffer::remove_download(Download_Stop);
	/*
	It is possible that terminating a download can trigger starting of
	another download. If this happens the stop function will return that
	download and the servers associated with that download. If the new
	download triggered has any of the same servers associated with it
	the DC's for that server will be added to client_buffers so that no
	reconnecting has to be done.
	*/
	std::list<download_connection> servers;
	download * Download_Start;
	if(Download_Factory.stop(Download_Stop, Download_Start, servers)){
		client_buffer::add_download(Download_Start);
		std::list<download_connection>::iterator iter_cur, iter_end;
		iter_cur = servers.begin();
		iter_end = servers.end();
		while(iter_cur != iter_end){
			Client_New_Connection.queue(*iter_cur);
			++iter_cur;
		}
	}
}
