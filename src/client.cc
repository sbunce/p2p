#include "client.h"

client::client()
:
	FD_max(0),
	Download_Factory(),
	stop_threads(false),
	threads(0),
	Thread_Pool(global::NEW_CONN_THREADS)
{
	//create the download directory if it doesn't exist
	boost::filesystem::create_directory(global::DOWNLOAD_DIRECTORY);

	//socket bitvector must be initialized before use
	FD_ZERO(&master_FDS);

	Speed_Calculator.set_speed_limit(DB_Client_Preferences.get_speed_limit_uint());
}

client::~client()
{
	for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
		if(FD_ISSET(socket_FD, &master_FDS)){
			disconnect(socket_FD);
		}
	}
	client_buffer::destroy();
}

void client::check_timeouts()
{
	std::vector<int> timed_out;
	client_buffer::check_timeouts(timed_out);
	std::vector<int>::iterator iter_cur, iter_end;
	iter_cur = timed_out.begin();
	iter_end = timed_out.end();
	while(iter_cur != iter_end){
		disconnect(*iter_cur);
		++iter_cur;
	}
}

void client::current_downloads(std::vector<download_info> & info)
{
	client_buffer::current_downloads(info);
}

void client::DC_block_concurrent(download_conn * DC)
{
	while(true){
		{
		boost::mutex::scoped_lock lock(CCA_mutex);
		bool found = false;
		std::list<download_conn *>::iterator iter_cur, iter_end;
		iter_cur = Connection_Current_Attempt.begin();
		iter_end = Connection_Current_Attempt.end();
		while(iter_cur != iter_end){
			if((*iter_cur)->IP == DC->IP){
				found = true;
				break;
			}
			++iter_cur;
		}
		if(!found){
			Connection_Current_Attempt.push_back(DC);
			break;
		}
		}
		sleep(1);
	}
}

void client::DC_unblock(download_conn * DC)
{
	boost::mutex::scoped_lock lock(CCA_mutex);
	std::list<download_conn *>::iterator iter_cur, iter_end;
	iter_cur = Connection_Current_Attempt.begin();
	iter_end = Connection_Current_Attempt.end();
	while(iter_cur != iter_end){
		if(*iter_cur == DC){
			Connection_Current_Attempt.erase(iter_cur);
			break;
		}
		++iter_cur;
	}
}

inline void client::disconnect(const int & socket_FD)
{
	logger::debug(LOGGER_P1,"disconnecting socket ",socket_FD);

	close(socket_FD);
	FD_CLR(socket_FD, &master_FDS);
	FD_CLR(socket_FD, &read_FDS);
	FD_CLR(socket_FD, &write_FDS);

	//reduce FD_max if possible
	for(int x = FD_max; x != 0; --x){
		if(FD_ISSET(x, &master_FDS)){
			FD_max = x;
			break;
		}
	}
}

int client::get_max_connections()
{
	return DB_Client_Preferences.get_max_connections();
}

void client::set_max_connections(int max_connections)
{
	DB_Client_Preferences.set_max_connections(max_connections);
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
	sl << Speed_Calculator.get_speed_limit() / 1024;
	return sl.str();
}

void client::set_speed_limit(const std::string & speed_limit)
{
	std::stringstream ss(speed_limit);
	unsigned int speed;
	ss >> speed;
	speed *= 1024;
	DB_Client_Preferences.set_speed_limit(speed);
	Speed_Calculator.set_speed_limit(speed);
}

void client::main_thread()
{
	++threads;

	//reconnect downloads that havn't finished
	std::list<download_info> resumed_download;
	DB_Download.initial_fill_buff(resumed_download);
	std::list<download_info>::iterator iter_cur, iter_end;
	iter_cur = resumed_download.begin();
	iter_end = resumed_download.end();
	while(iter_cur != iter_end){
		start_download(*iter_cur);
		++iter_cur;
	}

	int recv_limit = 0;
	while(true){
		if(stop_threads){
			break;
		}

		check_timeouts();
		remove_complete();
		client_buffer::generate_requests();

		/*
		These must be initialized every iteration on linux(and possibly other OS's)
		because linux will change them(POSIX.1-2001 allows this) to reflect the
		time that select() has blocked for.
		*/
		timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		if(client_buffer::get_send_pending() != 0){
			read_FDS = master_FDS;
			write_FDS = master_FDS;
			if((select(FD_max+1, &read_FDS, &write_FDS, NULL, &tv)) == -1){
				if(errno != EINTR){ //EINTR is caused by gprof
					perror("client select");
					exit(1);
				}
			}
		}
		else{
			FD_ZERO(&write_FDS);
			read_FDS = master_FDS;
			if((select(FD_max+1, &read_FDS, NULL, NULL, &tv)) == -1){
				if(errno != EINTR){ //EINTR is caused by gprof
					perror("client select");
					exit(1);
				}
			}
		}

		//process reads/writes
		char recv_buff[global::C_MAX_SIZE*global::PIPELINE_SIZE];
		int n_bytes;
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &read_FDS)){
				if((recv_limit = Speed_Calculator.rate_control(global::C_MAX_SIZE*global::PIPELINE_SIZE)) != 0){
					if((n_bytes = recv(socket_FD, recv_buff, recv_limit, MSG_NOSIGNAL)) <= 0){
						if(n_bytes == -1){
							perror("client recv");
						}
						disconnect(socket_FD);
						client_buffer::erase(socket_FD);
					}else{
						Speed_Calculator.update(n_bytes);
						client_buffer::get_recv_buff(socket_FD).append(recv_buff, n_bytes);
						client_buffer::post_recv(socket_FD);
					}
				}
			}
			if(FD_ISSET(socket_FD, &write_FDS)){
				if(!client_buffer::get_send_buff(socket_FD).empty()){
					if((n_bytes = send(socket_FD, client_buffer::get_send_buff(socket_FD).c_str(), client_buffer::get_send_buff(socket_FD).size(), MSG_NOSIGNAL)) < 0){
						if(n_bytes == -1){
							perror("client send");
						}
					}else{
						//remove bytes sent from buffer
						client_buffer::get_send_buff(socket_FD).erase(0, n_bytes);
						client_buffer::post_send(socket_FD);
					}
				}
			}
		}
	}

	--threads;
}

bool client::known_unresponsive(const std::string & IP)
{
	boost::mutex::scoped_lock lock(KU_mutex);

	//remove elements that are too old
	Known_Unresponsive.erase(Known_Unresponsive.begin(),
		Known_Unresponsive.upper_bound(time(0) - global::UNRESPONSIVE_TIMEOUT));

	//see if IP is in the list
	std::map<time_t,std::string>::iterator iter_cur, iter_end;
	iter_cur = Known_Unresponsive.begin();
	iter_end = Known_Unresponsive.end();
	while(iter_cur != iter_end){
		if(iter_cur->second == IP){
			return true;
		}
		else{
			++iter_cur;
		}
	}
	return false;
}

void client::new_conn(download_conn * DC)
{
	/*
	If this DC is for a server that is known to have rejected a previous
	connection attempt to it within the last minute then don't attempt another
	connection.
	*/
	if(known_unresponsive(DC->IP)){
		logger::debug(LOGGER_P1,"stopping connection to known unresponsive server ",DC->IP);
		return;
	}

	//this mess is a reentrant version of gethostname that is added in glibc
	hostent host_buf, *he;
	char * tmp_host_buf;
	int result_code, herr, host_buf_len;
	host_buf_len = 1024;
	tmp_host_buf = (char *)malloc(host_buf_len);
	while((result_code = gethostbyname_r(DC->IP.c_str(), &host_buf, tmp_host_buf, host_buf_len, &he, &herr)) == ERANGE){
		host_buf_len *= 2;
		tmp_host_buf = (char *)realloc(tmp_host_buf, host_buf_len);
	}

	if(he == NULL || strncmp(inet_ntoa(*(struct in_addr*)he->h_addr), "127", 3) == 0){
		boost::mutex::scoped_lock lock(KU_mutex);
		logger::debug(LOGGER_P1,"stopping connection to localhost");
		Known_Unresponsive.insert(std::make_pair(time(0), DC->IP));
		free(tmp_host_buf);
		return;
	}

	sockaddr_in dest_addr;
	DC->socket = socket(PF_INET, SOCK_STREAM, 0);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(global::P2P_PORT);
	dest_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
	memset(&(dest_addr.sin_zero),'\0',8);
	if(connect(DC->socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0){
		logger::debug(LOGGER_P1,"connection to ",DC->IP," failed");
		boost::mutex::scoped_lock lock(KU_mutex);
		Known_Unresponsive.insert(std::make_pair(time(0), DC->IP));
	}else{
		boost::mutex::scoped_lock lock(CCA_mutex);
		logger::debug(LOGGER_P1,"created socket ",DC->socket," for ",DC->IP);
		if(DC->socket > FD_max){
			FD_max = DC->socket;
		}

		if(DC->Download == NULL){
			//connetion cancelled during connection, clean up and exit
			disconnect(DC->socket);
		}else{
			client_buffer::add_download(DC->Download);
			client_buffer::new_connection(DC);
			FD_SET(DC->socket, &master_FDS);
		}
	}
	free(tmp_host_buf);
}

void client::process_CQ()
{
	download_conn * DC;
	{//begin lock scope
	boost::mutex::scoped_lock lock(CQ_mutex);
	if(Connection_Queue.empty()){
std::cout << "CQ empty\n";
		exit(1);
	}
	DC = Connection_Queue.front();
	Connection_Queue.pop_front();
	}//end lock scope

	//do not allow multiple DC with the same server_IP past
	DC_block_concurrent(DC);

	/*
	Add DC to an existing connection. Everything in here needs to account for the
	possibility of a download getting cancelled mid connection. If the download
	pointer in DC->Download is set to NULL it indicates a cancelled connection.
	*/
	if(!client_buffer::DC_add_existing(DC)){
std::cout << DC->Download->name() << " " <<  DC->IP << " making new conn\n";
		new_conn(DC);
	}

	//allow DC_block_concurrent to send next DC with same server_IP
	DC_unblock(DC);
}

void client::remove_complete()
{
	std::list<download *> complete;
	client_buffer::find_complete(complete);
	if(complete.empty()){
		return;
	}

	remove_pending_DC(complete);

	std::list<download *>::iterator C_iter_cur, C_iter_end;
	C_iter_cur = complete.begin();
	C_iter_end = complete.end();
	while(C_iter_cur != C_iter_end){
		transition_download(*C_iter_cur);
		++C_iter_cur;
	}

	//disconnect any empty client_buffers
	std::vector<int> disconnect_sockets;
	client_buffer::remove_empty(disconnect_sockets);
	std::vector<int>::iterator iter_cur, iter_end;
	iter_cur = disconnect_sockets.begin();
	iter_end = disconnect_sockets.end();
	while(iter_cur != iter_end){
		disconnect(*iter_cur);
		++iter_cur;
	}
}

void client::remove_pending_DC(std::list<download *> & complete)
{
	/*
	Remove any pending_connections for download. In progress connections will be
	cancelled by setting their download pointer to NULL. The new_conn function
	will interpret this NULL and will not establish a connection.
	*/
	std::list<download *>::iterator C_iter_cur, C_iter_end;
	C_iter_cur = complete.begin();
	C_iter_end = complete.end();
	while(C_iter_cur != C_iter_end){
		//pending connections
		boost::mutex::scoped_lock lock_1(CQ_mutex);
		std::list<download_conn *>::iterator CQ_iter_cur, CQ_iter_end;
		CQ_iter_cur = Connection_Queue.begin();
		CQ_iter_end = Connection_Queue.end();
		while(CQ_iter_cur != CQ_iter_end){
			if((*CQ_iter_cur)->Download == *C_iter_cur){
std::cout << "set DC->Download to NULL\n";
				(*CQ_iter_cur)->Download = NULL;
			}
			++CQ_iter_cur;
		}
		//in progress connections
		boost::mutex::scoped_lock lock_2(CCA_mutex);
		std::list<download_conn *>::iterator CCA_iter_cur, CCA_iter_end;
		CCA_iter_cur = Connection_Current_Attempt.begin();
		CCA_iter_end = Connection_Current_Attempt.end();
		while(CCA_iter_cur != CCA_iter_end){
			if((*CCA_iter_cur)->Download == *C_iter_cur){
std::cout << "set DC->Download to NULL\n";
				(*CCA_iter_cur)->Download = NULL;
			}
			++CCA_iter_cur;
		}
		++C_iter_cur;
	}
}

void client::search(std::string search_word, std::vector<download_info> & Search_Info)
{
	DB_Search.search(search_word, Search_Info);
}

void client::start()
{
	assert(threads == 0);
	boost::thread T1(boost::bind(&client::main_thread, this));
}

void client::stop()
{
	stop_threads = true;
	Thread_Pool.stop();
	while(threads){
		usleep(1);
	}
}

bool client::start_download(download_info & info)
{
	download * Download;
	std::list<download_conn *> servers;
	if(!Download_Factory.start_hash(info, Download, servers)){
		return false;
	}

	if(Download->complete()){
std::cout << "Download complete upon start\n";
		//file hash complete start download_file
		transition_download(Download);
		return true;
	}

	client_buffer::add_download(Download);

	//queue jobs to connect to servers
	std::list<download_conn *>::iterator iter_cur, iter_end;
	iter_cur = servers.begin();
	iter_end = servers.end();
	while(iter_cur != iter_end){
		boost::mutex::scoped_lock lock(CQ_mutex);
		Connection_Queue.push_back(*iter_cur);
		++iter_cur;
	}

	for(int x = 0; x<servers.size(); ++x){
		Thread_Pool.queue_job(this, &client::process_CQ);
	}

	return true;
}

void client::stop_download(std::string hash)
{
	client_buffer::stop_download(hash);
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
	std::list<download_conn *> servers;
	download * Download_Start;
	if(Download_Factory.stop(Download_Stop, Download_Start, servers)){
std::cout << "Download triggered start of another download\n";
		client_buffer::add_download(Download_Start);

		//attempt to place all servers in a exiting client_buffer
		std::list<download_conn *>::iterator iter_cur, iter_end;
		iter_cur = servers.begin();
		iter_end = servers.end();
		while(iter_cur != iter_end){
			if(client_buffer::DC_add_existing(*iter_cur)){
std::cout << "found existing server\n";
				iter_cur = servers.erase(iter_cur);
			}else{
				++iter_cur;
			}
		}

std::cout << "need to connect to " << servers.size() << " servers\n";

		//connect to servers that weren't already connected
		iter_cur = servers.begin();
		iter_end = servers.end();
		while(iter_cur != iter_end){
			boost::mutex::scoped_lock lock_1(CQ_mutex);
			Connection_Queue.push_back(*iter_cur);
			++iter_cur;
		}

		for(int x = 0; x<servers.size(); ++x){
			Thread_Pool.queue_job(this, &client::process_CQ);
		}
	}
}
