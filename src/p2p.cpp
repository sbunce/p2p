#include "p2p.hpp"

p2p::p2p()
{
	boost::filesystem::create_directory(settings::DOWNLOAD_DIRECTORY);
	rate_limit::singleton().set_max_download_rate(DB_Preferences.get_max_download_rate());
	rate_limit::singleton().set_max_upload_rate(DB_Preferences.get_max_upload_rate());

	#ifdef WIN32
	WORD wsock_ver = MAKEWORD(1,1);
	WSADATA wsock_data;
	int startup;
	if((startup = WSAStartup(wsock_ver, &wsock_data)) != 0){
		LOGGER << "winsock startup error " << startup;
		exit(1);
	}
	#endif

	main_thread = boost::thread(boost::bind(&p2p::main_loop, this));
}

p2p::~p2p()
{
	main_thread.interrupt();
	main_thread.join();
	#ifdef WIN32
	WSACleanup();
	#endif
	p2p_buffer::destroy_all();
}

void p2p::check_blacklist(int & blacklist_state)
{
	if(DB_Blacklist.modified(blacklist_state)){
		sockaddr_in temp_addr;
		socklen_t len = sizeof(temp_addr);
		for(int socket_FD = 0; socket_FD <= sockets::singleton().FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &sockets::singleton().master_FDS)){
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

void p2p::check_timeouts()
{
	int tmp;
	while((tmp = sockets::singleton().timed_out()) != -1){
		LOGGER << "socket " << tmp << " timed out";
		disconnect(tmp);
	}
}

void p2p::current_downloads(std::vector<download_status> & info, std::string hash)
{
	p2p_buffer::current_downloads(info, hash);
}

void p2p::current_uploads(std::vector<upload_info> & info)
{
	p2p_buffer::current_uploads(info);
}

void p2p::disconnect(const int & socket_FD)
{
	LOGGER << "disconnecting socket " << socket_FD;

	if(socket_FD == sockets::singleton().localhost_socket){
		sockets::singleton().localhost_socket = -1;
	}

	#ifdef WIN32
	closesocket(socket_FD);
	#else
	close(socket_FD);
	#endif

	--sockets::singleton().connections;
	FD_CLR(socket_FD, &sockets::singleton().master_FDS);
	FD_CLR(socket_FD, &sockets::singleton().read_FDS);
	FD_CLR(socket_FD, &sockets::singleton().write_FDS);
	p2p_buffer::erase(socket_FD);

	//reduce FD_max if possible
	for(int x=sockets::singleton().FD_max; x>0; --x){
		if(FD_ISSET(x, &sockets::singleton().master_FDS)){
			sockets::singleton().FD_max = x;
			break;
		}
	}
}

bool p2p::file_info(const std::string & hash, std::string & path, boost::uint64_t & tree_size, boost::uint64_t & file_size)
{
	if(DB_Download.lookup_hash(hash, path, file_size)){
		tree_size = hash_tree::tree_info::file_size_to_tree_size(file_size);
		return true;
	}else{
		return false;
	}
}

void p2p::pause_download(const std::string & hash)
{
	p2p_buffer::pause_download(hash);
}

unsigned p2p::prime_count()
{
	return number_generator::singleton().prime_count();
}

unsigned p2p::get_max_connections()
{
	return sockets::singleton().max_connections;
}

std::string p2p::get_download_directory()
{
	return DB_Preferences.get_download_directory();
}

std::string p2p::get_share_directory()
{
	return DB_Preferences.get_share_directory();
}

unsigned p2p::get_max_download_rate()
{
	if(rate_limit::singleton().get_max_download_rate() == std::numeric_limits<unsigned>::max()){
		return 0;
	}else{
		return rate_limit::singleton().get_max_download_rate();
	}
}

unsigned p2p::get_max_upload_rate()
{
	if(rate_limit::singleton().get_max_upload_rate() == std::numeric_limits<unsigned>::max()){
		return 0;
	}else{
		return rate_limit::singleton().get_max_upload_rate();
	}
}

bool p2p::is_indexing()
{
	return share_index::singleton().is_indexing();
}

void p2p::new_connection(const int & listener)
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

	if(new_IP.find("127") == 0){
		sockets::singleton().localhost_socket = new_FD;
	}

	//make sure not already connected
	sockaddr_in temp_addr;
	for(int socket_FD = 0; socket_FD <= sockets::singleton().FD_max; ++socket_FD){
		if(FD_ISSET(socket_FD, &sockets::singleton().master_FDS)){
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

	if(sockets::singleton().connections < sockets::singleton().max_connections){
		++sockets::singleton().connections;
		p2p_buffer::add_connection(new_FD, new_IP, false);
		FD_SET(new_FD, &sockets::singleton().master_FDS);
		if(new_FD > sockets::singleton().FD_max){
			sockets::singleton().FD_max = new_FD;
		}
		sockets::singleton().update_active(new_FD);
		LOGGER << "p2p " << inet_ntoa(remoteaddr.sin_addr) << " socket " << new_FD << " connected";
	}else{
		#ifdef WIN32
		closesocket(new_FD);
		#else
		close(new_FD);
		#endif
	}

	return;
}

int p2p::setup_listener()
{
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
	myaddr.sin_port = htons(settings::P2P_PORT); //set listening port
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

	FD_SET(listener, &sockets::singleton().master_FDS);
	sockets::singleton().FD_max = listener;
	return listener;
}

void p2p::main_loop()
{
	//heavy processing to bring this thread up, sleep to give GUI time to start
	portable_sleep::ms(2000);

	reconnect_unfinished();
	number_generator::init();
	share_index::init();

	int listener = setup_listener();

	//see documentation in database_table_blacklist to see what this does
	int blacklist_state = 0;

	/*
	Holds a copy of a chunk of the send buffer within the p2p_buffer.
	This buffer is needed so that the p2p_buffer can be fully thread
	safe. If we were reading from the send_buff within the p2p_buffer
	and someone called p2p_buffer::erase on it then we'd have trouble.
	*/
	std::string send_buff;
	const int max_buff = protocol::MAX_MESSAGE_SIZE * protocol::PIPELINE_SIZE;
	send_buff.reserve(max_buff);

	char recv_buff[max_buff]; //buff recv writes to
	int n_bytes;              //how many bytes sent or received by send()/recv()
	unsigned transfer_limit;  //speed_calculator stores how much may be recv'd here
	bool transfer;            //triggers sleep if nothing sent or received last iteration
	time_t Time(0);           //used to limit selected function calls to 1 per second max
	timeval tv;

	//see documentation right above socket checking loop
	unsigned max_send = 0;
	unsigned max_recv = 0;

	while(true){
		boost::this_thread::interruption_point();
		p2p_buffer::process();
		check_blacklist(blacklist_state); //check/disconnect blacklisted IPs
		if(Time != std::time(NULL)){
			check_timeouts();          //check/disconnect timed out sockets
			remove_complete();         //remove completed downloads
			start_pending_downloads(); //start queued downloads
			Time = std::time(NULL);
		}

		/*
		These must be initialized every iteration on linux(and possibly other OS's)
		because linux will change them to reflect the time that select() has
		blocked(POSIX.1-2001 allows this) for.
		*/
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		#ifdef WIN32
		//winsock doesn't allow calling select() with empty socket set
		if(sockets::singleton().master_FDS.fd_count == 0){
			portable_sleep::ms(1000);
			continue;
		}
		#endif

		if(p2p_buffer::get_send_pending() != 0){
			sockets::singleton().read_FDS = sockets::singleton().master_FDS;
			sockets::singleton().write_FDS = sockets::singleton().master_FDS;
			if(select(sockets::singleton().FD_max+1, &sockets::singleton().read_FDS,
				&sockets::singleton().write_FDS, NULL, &tv) == -1)
			{
				if(errno != EINTR){ //EINTR is caused by gprof
					#ifdef WIN32
					LOGGER << "winsock error " << WSAGetLastError();
					#else
					perror("p2p select");
					#endif
					exit(1);
				}
			}
		}else{
			FD_ZERO(&sockets::singleton().write_FDS);
			sockets::singleton().read_FDS = sockets::singleton().master_FDS;
			if(select(sockets::singleton().FD_max+1, &sockets::singleton().read_FDS, NULL, NULL, &tv) == -1){
				if(errno != EINTR){ //EINTR is caused by gprof
					#ifdef WIN32
					LOGGER << "winsock error " << WSAGetLastError();
					#else
					perror("p2p select");
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
		if(sockets::singleton().connections == 0){
			max_send = 1;
			max_recv = 1;
		}else{
			/*
			rate_limit::get_download_rate() returns unsigned int, the recv() function
			take a signed int. If max_recv is beyond the max size of an int then
			the conversion can yield a negative number.

			This is fixed here because it's nice to have an unsigned everywhere
			else because rates cannot logically be negative.
			*/
			unsigned tmp;
			if((tmp = rate_limit::singleton().get_max_upload_rate() / sockets::singleton().connections) > std::numeric_limits<int>::max()){
				max_send = std::numeric_limits<int>::max();
			}else{
				max_send = tmp;
			}
			if((tmp = rate_limit::singleton().get_max_download_rate() / sockets::singleton().connections) > std::numeric_limits<int>::max()){
				max_recv = std::numeric_limits<int>::max();
			}else{
				max_recv = tmp;
			}
		}

		//process reads/writes
		transfer = false;
		for(int socket_FD = 0; socket_FD <= sockets::singleton().FD_max; ++socket_FD){
			boost::this_thread::interruption_point();
			if(FD_ISSET(socket_FD, &sockets::singleton().read_FDS)){
				if(socket_FD == listener){
					new_connection(listener);
				}else{
					if((transfer_limit = rate_limit::singleton().download_rate_control(max_buff)) != 0){
						if(socket_FD == sockets::singleton().localhost_socket){
							transfer_limit = max_buff;
						}else if(transfer_limit > max_recv){
							transfer_limit = max_recv;
						}
						if((n_bytes = recv(socket_FD, recv_buff, transfer_limit, MSG_NOSIGNAL)) <= 0){
							if(n_bytes == -1){
								#ifdef WIN32
								LOGGER << "winsock error " << WSAGetLastError();
								#else
								perror("p2p recv");
								#endif
							}
							LOGGER << "remote " << socket_FD << " requested disconnect";
							disconnect(socket_FD);
							continue;
						}else{
							sockets::singleton().update_active(socket_FD);
							if(socket_FD != sockets::singleton().localhost_socket){
								rate_limit::singleton().add_download_bytes(n_bytes);
							}
							p2p_buffer::post_recv(socket_FD, recv_buff, n_bytes);
							transfer = true;
						}
					}
				}
			}

			if(FD_ISSET(socket_FD, &sockets::singleton().write_FDS)){
				if(p2p_buffer::get_send_buff(socket_FD, max_send, send_buff)){
					if((transfer_limit = rate_limit::singleton().upload_rate_control(send_buff.size())) != 0){
						if(socket_FD == sockets::singleton().localhost_socket){
							transfer_limit = send_buff.size();
						}else if(transfer_limit > max_send){
							transfer_limit = max_send;
						}
						if((n_bytes = send(socket_FD, send_buff.data(), transfer_limit, MSG_NOSIGNAL)) <= 0){
							if(n_bytes == -1){
								#ifdef WIN32
								LOGGER << "winsock error " << WSAGetLastError();
								#else
								perror("p2p send");
								#endif
							}
							LOGGER << "remote " << socket_FD << " requested disconnect";
							disconnect(socket_FD);
						}else{
							sockets::singleton().update_active(socket_FD);
							if(socket_FD != sockets::singleton().localhost_socket){
								rate_limit::singleton().add_upload_bytes(n_bytes);
							}
							if(p2p_buffer::post_send(socket_FD, n_bytes)){
								LOGGER << "post send disconnect of socket " << socket_FD;
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

void p2p::reconnect_unfinished()
{
	std::vector<download_info> resume;
	DB_Download.resume(resume);
	std::vector<download_info>::iterator iter_cur, iter_end;
	iter_cur = resume.begin();
	iter_end = resume.end();
	while(iter_cur != iter_end){
		start_download_process(*iter_cur);
		++iter_cur;
	}
}

void p2p::remove_complete()
{
	std::vector<boost::shared_ptr<download> > complete;
	p2p_buffer::get_complete_downloads(complete);
	if(complete.empty()){
		return;
	}

	std::vector<boost::shared_ptr<download> >::iterator C_iter_cur, C_iter_end;
	C_iter_cur = complete.begin();
	C_iter_end = complete.end();
	while(C_iter_cur != C_iter_end){
		transition_download(*C_iter_cur);
		++C_iter_cur;
	}
}

void p2p::search(std::string search_word, std::vector<download_info> & Search_Info)
{
	DB_Search.run_search(search_word, Search_Info);
}

void p2p::set_max_connections(const unsigned & max_connections_in)
{
	sockets::singleton().max_connections = max_connections_in;
	DB_Preferences.set_max_connections(sockets::singleton().max_connections);
	while(sockets::singleton().connections > sockets::singleton().max_connections){
		for(int socket_FD = 0; socket_FD <= sockets::singleton().FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &sockets::singleton().master_FDS)){
				disconnect(socket_FD);
				break;
			}
		}
	}
}

void p2p::set_download_directory(const std::string & download_directory)
{
	DB_Preferences.set_download_directory(download_directory);
}

void p2p::set_share_directory(const std::string & share_directory)
{
	DB_Preferences.set_share_directory(share_directory);
	std::cout << "SETTING SHARE FEATURE UNIMPLEMENTED\n";
}

void p2p::set_max_download_rate(unsigned download_rate)
{
	if(download_rate == 0){
		download_rate = std::numeric_limits<unsigned>::max();
	}
	DB_Preferences.set_max_download_rate(download_rate);
	rate_limit::singleton().set_max_download_rate(download_rate);
}

void p2p::set_max_upload_rate(unsigned upload_rate)
{
	if(upload_rate == 0){
		upload_rate = std::numeric_limits<unsigned>::max();
	}
	DB_Preferences.set_max_upload_rate(upload_rate);
	rate_limit::singleton().set_max_upload_rate(upload_rate);
}

void p2p::start_download(const download_info & info)
{
	boost::mutex::scoped_lock lock(PD_mutex);
	Pending_Download.push_back(info);
}

void p2p::remove_download(const std::string & hash)
{
	p2p_buffer::remove_download(hash);
}

void p2p::start_download_process(const download_info & info)
{
	boost::shared_ptr<download> Download;
	std::vector<download_connection> servers;
	Download = Download_Factory.start(info, servers);
	if(Download.get() != NULL){
		p2p_buffer::add_download(Download);
		std::vector<download_connection>::iterator iter_cur, iter_end;
		iter_cur = servers.begin();
		iter_end = servers.end();
		while(iter_cur != iter_end){
			p2p_new_connection::singleton().queue(*iter_cur);
			++iter_cur;
		}
	}
}

void p2p::start_pending_downloads()
{
	/*
	The start_download_process function is slow so we copy all the download_info
	so the Pending_Download container can be unlocked as soon as possible.
	*/
	std::list<download_info> Pending_Download_Temp;

	{
	boost::mutex::scoped_lock lock(PD_mutex);
	Pending_Download_Temp.assign(Pending_Download.begin(), Pending_Download.end());
	Pending_Download.clear();
	}

	std::list<download_info>::iterator iter_cur, iter_end;
	iter_cur = Pending_Download_Temp.begin();
	iter_end = Pending_Download_Temp.end();
	while(iter_cur != iter_end){
		//this is the slow function which neccessitates the copy
		start_download_process(*iter_cur);
		++iter_cur;
	}
}

unsigned p2p::download_rate()
{
	return rate_limit::singleton().current_download_rate();
}

unsigned p2p::upload_rate()
{
	return rate_limit::singleton().current_upload_rate();
}

void p2p::transition_download(boost::shared_ptr<download> Download_Stop)
{
	/*
	It is possible that terminating a download can trigger starting of
	another download. If this happens the stop function will return that
	download and the servers associated with that download. If the new
	download triggered has any of the same servers associated with it
	the DC's for that server will be added to p2p_buffers so that no
	reconnecting has to be done.
	*/
	std::vector<download_connection> servers;
	boost::shared_ptr<download> Download_Start;
	Download_Start = Download_Factory.stop(Download_Stop, servers);
	if(Download_Start.get() != NULL){
		p2p_buffer::add_download(Download_Start);
		std::vector<download_connection>::iterator iter_cur, iter_end;
		iter_cur = servers.begin();
		iter_end = servers.end();
		while(iter_cur != iter_end){
			p2p_new_connection::singleton().queue(*iter_cur);
			++iter_cur;
		}
	}
}
	
