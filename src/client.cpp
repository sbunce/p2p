#include "client.hpp"

client::client():
	connections(0),
	FD_max(0),
	Client_New_Connection(master_FDS, FD_max, max_connections, connections)
{
	boost::filesystem::create_directory(global::DOWNLOAD_DIRECTORY);
	max_connections = DB_Preferences.get_client_connections();
	FD_ZERO(&master_FDS);
	Rate_Limit.set_download_rate(DB_Preferences.get_download_rate());

	#ifdef WIN32
	WORD wsock_ver = MAKEWORD(1,1);
	WSADATA wsock_data;
	int startup;
	if((startup = WSAStartup(wsock_ver, &wsock_data)) != 0){
		LOGGER << "winsock startup error " << startup;
		exit(1);
	}
	#endif
	
	main_thread = boost::thread(boost::bind(&client::main_loop, this));

	//start prime generation
	number_generator::init();
}

client::~client()
{
	main_thread.interrupt();
	main_thread.join();
	client_buffer::destroy();

	#ifdef WIN32
	WSACleanup();
	#endif
}

void client::check_blacklist(int & blacklist_state)
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

void client::current_downloads(std::vector<download_status> & info, std::string hash)
{
	client_buffer::current_downloads(info, hash);
}

void client::disconnect(const int & socket_FD)
{
	LOGGER << "disconnecting socket " << socket_FD;

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
	for(int x=FD_max; x>0; --x){
		if(FD_ISSET(x, &master_FDS)){
			FD_max = x;
			break;
		}
	}
}

bool client::file_info(const std::string & hash, std::string & path, boost::uint64_t & tree_size, boost::uint64_t & file_size)
{
	if(DB_Download.lookup_hash(hash, path, file_size)){
		tree_size = hash_tree::file_size_to_tree_size(file_size);
		return true;
	}else{
		return false;
	}
}

unsigned client::prime_count()
{
	return number_generator::prime_count();
}

unsigned client::get_max_connections()
{
	return max_connections;
}

std::string client::get_download_directory()
{
	return DB_Preferences.get_download_directory();
}

unsigned client::get_download_rate()
{
	if(Rate_Limit.get_download_rate() == std::numeric_limits<unsigned>::max()){
		return 0;
	}else{
		return Rate_Limit.get_download_rate();
	}
}

void client::main_loop()
{
	reconnect_unfinished();

	//see documentation in database_table_blacklist to see what this does
	int blacklist_state = 0;

	char recv_buff[global::C_MAX_SIZE*global::PIPELINE_SIZE];
	int n_bytes;             //how many bytes sent or received by send()/recv()
	unsigned transfer_limit; //speed_calculator stores how much may be recv'd here
	bool transfer;           //triggers sleep if nothing sent or received last iteration
	time_t Time(0);          //used to limit selected function calls to 1 per second max
	unsigned max_recv = 0;   //see documentation right above socket checking loop
	timeval tv;

	while(true){
		boost::this_thread::interruption_point();
std::cout << "POIK1\n";
		client_buffer::generate_requests(); //let downloads generate requests
std::cout << "POIK2\n";
		check_blacklist(blacklist_state);   //check/disconnect blacklisted IPs
std::cout << "POIK3\n";
		if(Time != std::time(NULL)){
std::cout << "POIK4\n";
			check_timeouts();          //check/disconnect timed out sockets
std::cout << "POIK5\n";
			remove_complete();         //remove completed downloads
std::cout << "POIK6\n";
			remove_empty();            //remove empty client_buffers (no downloads)
std::cout << "POIK7\n";
			start_pending_downloads(); //start downloads queue'd by the GUI
std::cout << "POIK8\n";
			Time = std::time(NULL);

//DEBUG
std::cout << "open_speed:  " << database::connection::open_speed() << "\n";
std::cout << "query_speed: " << database::connection::query_speed() << "\n";
std::cout << "total:       " << database::connection::open_speed() + database::connection::query_speed() << "\n\n";
		}

		/*
		These must be initialized every iteration on linux(and possibly other OS's)
		because linux will change them(POSIX.1-2001 allows this) to reflect the
		time that select() has blocked for.
		*/
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		#ifdef WIN32
		//winsock doesn't allow calling select() with empty socket set
		if(master_FDS.fd_count == 0){
			portable_sleep::ms(1000);
			continue;
		}
		#endif
std::cout << "POIK9\n";
		if(client_buffer::get_send_pending() != 0){
			read_FDS = master_FDS;
			write_FDS = master_FDS;
			if(select(FD_max+1, &read_FDS, &write_FDS, NULL, &tv) == -1){
				if(errno != EINTR){ //EINTR is caused by gprof
					#ifdef WIN32
					LOGGER << "winsock error " << WSAGetLastError();
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
					LOGGER << "winsock error " << WSAGetLastError();
					#else
					perror("client select");
					#endif
					exit(1);
				}
			}
		}
std::cout << "POIK10\n";
		/*
		There is a starvation problem that happens when rate limiting. If all sockets
		are checked in order and the rate limit is lower than the total bandwidth
		then the higher socket numbers will get starved. To compensate for this
		we set a max_read = rate_limit / connections. This way, in the most demanding
		scenario each sockets gets to be read at least once. A lower limit on
		max_read is set at 1 of course.
		*/
		if(connections == 0){
			max_recv = 1;
		}else{
			/*
			Rate_Limit.get_download_rate() returns unsigned int, the recv() function
			take a signed int. If max_recv is beyond the max size of an int then
			the conversion can yield a negative number.

			This is fixed here because it's nice to have an unsigned everywhere
			else because rates cannot logically be negative.
			*/
			unsigned tmp;
			if((tmp = Rate_Limit.get_download_rate() / connections) > std::numeric_limits<int>::max()){
				max_recv = std::numeric_limits<int>::max();
			}else{
				max_recv = tmp;
			}
		}
std::cout << "POIK11\n";
		//process reads/writes
		transfer = false;
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
std::cout << "POIK12\n";
			if(FD_ISSET(socket_FD, &read_FDS)){
std::cout << "POIK13\n";
				if((transfer_limit = Rate_Limit.download_rate_control(global::C_MAX_SIZE*global::PIPELINE_SIZE)) != 0){
					if(transfer_limit > max_recv){
						transfer_limit = max_recv;
					}
std::cout << "POIK14\n";
					if((n_bytes = recv(socket_FD, recv_buff, transfer_limit, MSG_NOSIGNAL)) <= 0){
						if(n_bytes == -1){
							#ifdef WIN32
							LOGGER << "winsock error " << WSAGetLastError();
							#else
							perror("client recv");
							#endif
						}
						disconnect(socket_FD);
					}else{
std::cout << "POIK15\n";
						Rate_Limit.add_download_bytes(n_bytes);
std::cout << "POIK16\n";
						client_buffer::recv_buff_append(socket_FD, recv_buff, n_bytes);
std::cout << "POIK17\n";
						transfer = true;
					}
std::cout << "POIK18\n";
				}
std::cout << "POIK19\n";
			}
std::cout << "POIK20\n";
			if(FD_ISSET(socket_FD, &write_FDS)){
std::cout << "POIK21\n";
				std::string * buff = &client_buffer::get_send_buff(socket_FD);
std::cout << "POIK22\n";
				if(!buff->empty()){
std::cout << "POIK23\n";
					if((n_bytes = send(socket_FD, buff->c_str(), buff->size(), MSG_NOSIGNAL)) < 0){
std::cout << "POIK24\n";
						if(n_bytes == -1){
							#ifdef WIN32
							LOGGER << "winsock error " << WSAGetLastError();
							#else
							perror("client send");
							#endif
						}
					}else{
std::cout << "POIK25\n";
						//remove bytes sent from buffer
						Rate_Limit.add_upload_bytes(n_bytes);
std::cout << "POIK26\n";
						buff->erase(0, n_bytes);
						client_buffer::post_send(socket_FD);
std::cout << "POIK27\n";
						transfer = true;
					}
				}
std::cout << "POIK28\n";
			}
std::cout << "POIK29\n";
		}
std::cout << "POIK30\n";
		if(!transfer){
			boost::this_thread::yield();
		}
	}
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
	client_server_bridge::unblock_server_index();
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
	DB_Search.run_search(search_word, Search_Info);
}

void client::set_max_connections(const unsigned & max_connections_in)
{
	max_connections = max_connections_in;
	DB_Preferences.set_client_connections(max_connections);
	while(connections > max_connections){
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &master_FDS)){
				disconnect(socket_FD);
				break;
			}
		}
	}
}

void client::set_download_directory(const std::string & download_directory)
{
	DB_Preferences.set_download_directory(download_directory);
}

void client::set_max_download_rate(unsigned download_rate)
{
	if(download_rate == 0){
		download_rate = std::numeric_limits<unsigned>::max();
	}
	DB_Preferences.set_download_rate(download_rate);
	Rate_Limit.set_download_rate(download_rate);
}

void client::start_download(const download_info & info)
{
	Pending_Download->push_back(info);
}

void client::stop_download(std::string hash)
{
	client_buffer::stop_download(hash);
}

void client::start_download_process(const download_info & info)
{
	download * Download;
	std::list<download_connection> servers;
	if(Download_Factory.start(info, Download, servers)){
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
	The start_download_process function is slow so we copy all the download_info
	so the Pending_Download container can be unlocked as soon as possible.
	*/
	std::list<download_info> Pending_Download_Temp;

	Pending_Download.lock();
	Pending_Download_Temp.assign(Pending_Download->begin(), Pending_Download->end());
	Pending_Download->clear();
	Pending_Download.unlock();

	std::list<download_info>::iterator iter_cur, iter_end;
	iter_cur = Pending_Download_Temp.begin();
	iter_end = Pending_Download_Temp.end();
	while(iter_cur != iter_end){
		//this is the slow function which neccessitates the copy
		start_download_process(*iter_cur);
		++iter_cur;
	}
}

unsigned client::total_rate()
{
	/*
	If there is no I/O the speed won't get updated. This will update the speed
	as long as the client cares to check it.
	*/
	Rate_Limit.add_download_bytes(0);
	return Rate_Limit.download_speed();
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
	
