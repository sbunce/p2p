#include "client.h"

client::client()
:
FD_max(0),
send_pending(new volatile int(0)),
download_complete(new volatile int(0)),
Download_Factory(download_complete),
stop_threads(false),
threads(0),
current_time(time(0)),
previous_time(time(0)),
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
			Client_Buffer.erase(socket_FD);
		}
	}

	std::list<download *>::iterator iter_cur, iter_end;
	iter_cur = Download_Buffer.begin();
	iter_end = Download_Buffer.end();
	while(iter_cur != iter_end){
		delete *iter_cur;
		++iter_cur;
	}

	delete send_pending;
	delete download_complete;
}

void client::check_timeouts()
{
	for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
		if(FD_ISSET(socket_FD, &master_FDS)){
			if(current_time - Client_Buffer[socket_FD]->get_last_seen() >= global::TIMEOUT){
				logger::debug(LOGGER_P1,"triggering timeout of socket ",socket_FD);
				disconnect(socket_FD);
				delete Client_Buffer[socket_FD];
				Client_Buffer.erase(socket_FD);
			}
		}
	}
}

void client::current_downloads(std::vector<download_info> & info)
{
	client_buffer::current_downloads(info);
}

bool client::DC_add_existing(download_conn * DC)
{
	std::map<int, client_buffer *>::iterator iter_cur, iter_end;
	iter_cur = Client_Buffer.begin();
	iter_end = Client_Buffer.end();
	while(iter_cur != iter_end){
		if(DC->server_IP == iter_cur->second->get_IP()){
			//mutex to guarantee DC->Download not set to NULL after download cancelled check
			boost::mutex::scoped_lock lock(DC_mutex);

			//if download cancelled don't add it
			if(DC->Download == NULL){
				return true;
			}

			//client_buffer for the server found
			DC->Download->reg_conn(iter_cur->first, DC);  //register connection with download
			iter_cur->second->add_download(DC->Download); //register connection
			return true;
		}
		++iter_cur;
	}

	return false;
}

void client::DC_block_concurrent(download_conn * DC)
{
	while(true){
		bool found = false;
		{//begin lock scope
		boost::mutex::scoped_lock lock(DC_mutex);
		std::list<download_conn *>::iterator iter_cur, iter_end;
		iter_cur = Connection_Current_Attempt.begin();
		iter_end = Connection_Current_Attempt.end();
		while(iter_cur != iter_end){
			if((*iter_cur)->server_IP == DC->server_IP){
				found = true;
				break;
			}
			++iter_cur;
		}

		if(!found){
			Connection_Current_Attempt.push_back(DC);
			break;
		}
		}//end lock scope

		usleep(global::SPINLOCK_TIME);
	}
}

void client::DC_unblock(download_conn * DC)
{
	boost::mutex::scoped_lock lock(DC_mutex);

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

		current_time = time(0);
		if(current_time - previous_time > global::TIMEOUT){
			check_timeouts();
			previous_time = current_time;
		}

		if(*download_complete != 0){
			remove_complete();
		}

		prepare_requests();

		/*
		These must be initialized every iteration on linux(and possibly other OS's)
		because linux will change them(POSIX.1-2001 allows this) to reflect the
		time that select() has blocked for.
		*/
		timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		/*
		This if(send_pending) exists to saves CPU time by not checking if sockets
		are ready to write when we don't need to write anything.
		*/
		if(*send_pending != 0){
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
		std::map<int, client_buffer *>::iterator CB_iter;
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &read_FDS)){
				if((recv_limit = Speed_Calculator.rate_control(global::C_MAX_SIZE*global::PIPELINE_SIZE)) != 0){
					if((n_bytes = recv(socket_FD, recv_buff, recv_limit, MSG_NOSIGNAL)) <= 0){
						if(n_bytes == -1){
							perror("client recv");
						}
						disconnect(socket_FD);
						Client_Buffer.erase(socket_FD);
					}else{
						Speed_Calculator.update(n_bytes);

						Client_Buffer[socket_FD]->recv_buff.append(recv_buff, n_bytes);
						Client_Buffer[socket_FD]->post_recv();
					}
				}
			}
			if(FD_ISSET(socket_FD, &write_FDS)){
				if(!Client_Buffer[socket_FD]->send_buff.empty()){
					if((n_bytes = send(socket_FD, Client_Buffer[socket_FD]->send_buff.c_str(), Client_Buffer[socket_FD]->send_buff.size(), MSG_NOSIGNAL)) < 0){
						if(n_bytes == -1){
							perror("client send");
						}
					}else{ //remove bytes sent from buffer
						Client_Buffer[socket_FD]->send_buff.erase(0, n_bytes);
						Client_Buffer[socket_FD]->post_send();
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
	if(known_unresponsive(DC->server_IP)){
		logger::debug(LOGGER_P1,"stopping connection to known unresponsive server ",DC->server_IP);
		return;
	}

	hostent * he;

	{//begin lock scope
	boost::mutex::scoped_lock lock(gethostbyname_mutex);
	//resolve hostname
	he = gethostbyname(DC->server_IP.c_str());
	}//end lock scope

	if(he == NULL){
		{//begin lock scope
		boost::mutex::scoped_lock lock(KU_mutex);
		Known_Unresponsive.insert(std::make_pair(time(0), DC->server_IP));
		}//end lock scope
		herror("gethostbyname");
		return;
	}else if(strncmp(inet_ntoa(*(struct in_addr*)he->h_addr), "127", 3) == 0){
		{//begin lock scope
		boost::mutex::scoped_lock lock(KU_mutex);
		Known_Unresponsive.insert(std::make_pair(time(0), DC->server_IP));
		}//end lock scope
		return;
	}

	sockaddr_in dest_addr;
	DC->socket_FD = socket(PF_INET, SOCK_STREAM, 0);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(global::P2P_PORT);
	dest_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
	memset(&(dest_addr.sin_zero),'\0',8);

	if(connect(DC->socket_FD, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0){
		logger::debug(LOGGER_P1,"connection to ",DC->server_IP," failed");

		{//begin lock scope
		boost::mutex::scoped_lock lock(KU_mutex);
		Known_Unresponsive.insert(std::make_pair(time(0), DC->server_IP));
		}//end lock scope

		return;
	}else{
		logger::debug(LOGGER_P1,"created socket ",DC->socket_FD," for ",DC->server_IP);
		if(DC->socket_FD > FD_max){
			FD_max = DC->socket_FD;
		}

		if(DC->Download == NULL){
			//connetion cancelled during connection, clean up and exit
			disconnect(DC->socket_FD);
			return;
		}else{
			Client_Buffer.insert(std::make_pair(DC->socket_FD, new client_buffer(DC->socket_FD, DC->server_IP, send_pending)));
			DC->Download->reg_conn(DC->socket_FD, DC);
			Client_Buffer[DC->socket_FD]->add_download(DC->Download);
		}

		FD_SET(DC->socket_FD, &master_FDS);
		return;
	}
}

void client::prepare_requests()
{
	std::map<int, client_buffer *>::iterator iter_cur, iter_end;
	iter_cur = Client_Buffer.begin();
	iter_end = Client_Buffer.end();
	while(iter_cur != iter_end){
		iter_cur->second->prepare_request();
		++iter_cur;
	}
}

void client::process_CQ()
{
	download_conn * DC;
	{//begin lock scope
	boost::mutex::scoped_lock lock(DC_mutex);
	if(Connection_Queue.empty()){
		return;
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
	if(!DC_add_existing(DC)){
		new_conn(DC);
	}

	//allow DC_block_concurrent to send next DC with same server_IP
	DC_unblock(DC);
}

void client::remove_complete()
{
	/*
	STEP 1:
	Locate any complete downloads.
	*/
	std::list<download *> complete;
	client_buffer::find_complete(complete);

	//decrement download_complete count for downloads that will be terminated
	*download_complete -= complete.size();

	{
	/*
	STEP 2:
	Remove any pending_connections for download. In progress connections will be
	cancelled by setting their download pointer to NULL. The new_conn function
	will interpret this NULL.
	*/
	std::list<download *>::iterator C_iter_cur, C_iter_end;
	C_iter_cur = complete.begin();
	C_iter_end = complete.end();
	while(C_iter_cur != C_iter_end){
		//pending connections
		std::deque<download_conn *>::iterator CQ_iter_cur, CQ_iter_end;
		CQ_iter_cur = Connection_Queue.begin();
		CQ_iter_end = Connection_Queue.end();
		while(CQ_iter_cur != CQ_iter_end){
			if((*CQ_iter_cur)->Download == *C_iter_cur){
				CQ_iter_cur = Connection_Queue.erase(CQ_iter_cur);
			}else{
				++CQ_iter_cur;
			}
		}

		//in progress connections
		std::list<download_conn *>::iterator CQA_iter_cur, CQA_iter_end;
		CQA_iter_cur = Connection_Current_Attempt.begin();
		CQA_iter_end = Connection_Current_Attempt.end();
		while(CQA_iter_cur != CQA_iter_end){
			if((*CQA_iter_cur)->Download == *C_iter_cur){
				//this must be locked both here and where the Download is checked for NULL
				boost::mutex::scoped_lock lock(DC_mutex);
				(*CQA_iter_cur)->Download = NULL;
			}else{
				++CQA_iter_cur;
			}
		}

		++C_iter_cur;
	}
	}

	{
	/*
	STEP 3:
	Terminate the completed downloads with all client_buffers.
	*/
	std::map<int, client_buffer *>::iterator CB_iter_cur, CB_iter_end;
	CB_iter_cur = Client_Buffer.begin();
	CB_iter_end = Client_Buffer.end();
	while(CB_iter_cur != CB_iter_end){
		std::list<download *>::iterator C_iter_cur, C_iter_end;
		C_iter_cur = complete.begin();
		C_iter_end = complete.end();
		while(C_iter_cur != C_iter_end){
			CB_iter_cur->second->terminate_download(*C_iter_cur);
			++C_iter_cur;
		}
		++CB_iter_cur;
	}
	}

	{
	/*
	STEP 4:
	Remove the download from the client_buffer unique download set and possibly
	start another download if one is triggered.
	*/
	std::list<download *>::iterator C_iter_cur, C_iter_end;
	C_iter_cur = complete.begin();
	C_iter_end = complete.end();
	while(C_iter_cur != C_iter_end){
		//remove download from the unique download set
		client_buffer::remove_download_unique(*C_iter_cur);

		/*
		It is possible that terminating a download can trigger starting of
		another download. If this happens the stop function will return that
		download and the servers associated with that download. If the new
		download triggered has any of the same servers associated with it
		the DC's for that server will be added to client_buffers so that no
		reconnecting has to be done.
		*/
		std::list<download_conn *> servers;
		download * Download_start;
		if(Download_Factory.stop(*C_iter_cur, Download_start, servers)){
			//attempt to place all servers in a exiting client_buffer
			std::list<download_conn *>::iterator iter_cur, iter_end;
			iter_cur = servers.begin();
			iter_end = servers.end();
			while(iter_cur != iter_end){
				if(DC_add_existing(*iter_cur)){
					iter_cur = servers.erase(iter_cur);
				}else{
					++iter_cur;
				}
			}

			//queue servers that don't have an existing client_buffer
			iter_cur = servers.begin();
			iter_end = servers.end();
			while(iter_cur != iter_end){
				Connection_Queue.push_back(*iter_cur);
				Thread_Pool.queue_job(this, &client::process_CQ);
				++iter_cur;
			}
		}
		++C_iter_cur;
	}
	}

	{
	/*
	STEP 5:
	Disconnect empty client_buffer's.
	*/
	std::map<int, client_buffer *>::iterator CB_iter_cur, CB_iter_end;
	CB_iter_cur = Client_Buffer.begin();
	CB_iter_end = Client_Buffer.end();
	while(CB_iter_cur != CB_iter_end){
		if(CB_iter_cur->second->empty()){
			disconnect(CB_iter_cur->first);
			Client_Buffer.erase(CB_iter_cur);
			CB_iter_cur = Client_Buffer.begin();
		}else{
			++CB_iter_cur;
		}
	}
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
		usleep(global::SPINLOCK_TIME);
	}
}

bool client::start_download(download_info & info)
{
	download * Download;
	std::list<download_conn *> servers;
	if(!Download_Factory.start_file(info, Download, servers)){
		return false;
	}

	client_buffer::add_download_unique(Download);

	//queue jobs to connect to servers
	std::list<download_conn *>::iterator iter_cur, iter_end;
	iter_cur = servers.begin();
	iter_end = servers.end();
	while(iter_cur != iter_end){
		{//begin lock scope
		boost::mutex::scoped_lock lock(DC_mutex);
		Connection_Queue.push_back(*iter_cur);
		}//end lock scope
		Thread_Pool.queue_job(this, &client::process_CQ);
		++iter_cur;
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
