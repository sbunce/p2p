//std
#include <iostream>
#include <fstream>
#include <sstream>

#include "client.h"

client::client()
{
	FD_max = 0;
	FD_ZERO(&master_FDS);
	send_pending = new atomic<int>;
	*send_pending = 0;
	download_complete = new atomic<bool>;
	*download_complete = false;
	stop_threads = false;
	threads = 0;
	current_time = time(0);
	previous_time = time(0);
}

client::~client()
{
	for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
		if(FD_ISSET(socket_FD, &master_FDS)){
			disconnect(socket_FD);
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
	{//begin lock scope
	boost::mutex::scoped_lock lock(CB_D_mutex);
	for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
		if(FD_ISSET(socket_FD, &master_FDS)){
			if(current_time - Client_Buffer[socket_FD]->get_last_seen() >= global::TIMEOUT){
				#ifdef DEBUG
				std::cout << "error: client::check_timeouts() triggering disconnect of socket " << socket_FD << "\n";
				#endif
				disconnect(socket_FD);
			}
		}
	}
	}//end lock scope
}

//this should only be used from within a CB_D_mutex.
inline void client::disconnect(const int & socket_FD)
{
	#ifdef DEBUG
	std::cout << "info: client disconnecting socket number " << socket_FD << "\n";
	#endif

	close(socket_FD);
	FD_CLR(socket_FD, &master_FDS);

	//reduce FD_max if possible
	for(int x = FD_max; x != 0; --x){
		if(FD_ISSET(x, &master_FDS)){
			FD_max = x;
			break;
		}
	}

	std::map<int, client_buffer *>::iterator CB_iter = Client_Buffer.find(socket_FD);

	if(CB_iter != Client_Buffer.end()){
		delete CB_iter->second;
		Client_Buffer.erase(socket_FD);
	}
}

bool client::get_download_info(std::vector<info_buffer> & download_info)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(CB_D_mutex);
	if(Download_Buffer.size() == 0){
		return false;
	}

	std::list<download *>::iterator iter_cur, iter_end;
	iter_cur = Download_Buffer.begin();
	iter_end = Download_Buffer.end();
	while(iter_cur != iter_end){
		info_buffer info;
		info.hash = (*iter_cur)->hash();
		(*iter_cur)->IP_list(info.server_IP);
		info.file_name = (*iter_cur)->name();
		info.file_size = (*iter_cur)->total_size();
		info.percent_complete = (*iter_cur)->percent_complete();
		info.speed = (*iter_cur)->speed();
		download_info.push_back(info);
		++iter_cur;
	}
	}//end lock scope

	return true;
}

int client::get_total_speed()
{
	int speed = 0;
	{//begin lock scope
	boost::mutex::scoped_lock lock(CB_D_mutex);
	std::list<download *>::iterator iter_cur, iter_end;
	iter_cur = Download_Buffer.begin();
	iter_end = Download_Buffer.end();
	while(iter_cur != iter_end){
		speed += (*iter_cur)->speed();
		++iter_cur;
	}
	}//end lock scope

	return speed;
}

void client::main_thread()
{
	++threads;

	//reconnect downloads that havn't finished
	std::list<exploration::info_buffer> resumed_download;
	Client_Index.initial_fill_buff(resumed_download);
	std::list<exploration::info_buffer>::iterator iter_cur, iter_end;
	iter_cur = resumed_download.begin();
	iter_end = resumed_download.end();
	while(iter_cur != iter_end){
		start_download(*iter_cur);
		++iter_cur;
	}

	while(true){
		if(stop_threads){
			break;
		}

		current_time = time(0);
		if(current_time - previous_time > global::TIMEOUT){
			check_timeouts();
			previous_time = current_time;
		}

		if(*download_complete){
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
		This if(send_pending) exists to save a LOT of CPU time by not checking
		if sockets are ready to write when we don't need to write anything.
		*/
		if(*send_pending != 0){
			read_FDS = master_FDS;
			write_FDS = master_FDS;
			if((select(FD_max+1, &read_FDS, &write_FDS, NULL, &tv)) == -1){
				if(errno != EINTR){ //EINTR is caused by gprof
					perror("select");
					exit(1);
				}
			}
		}
		else{
			FD_ZERO(&write_FDS);
			read_FDS = master_FDS;
			if((select(FD_max+1, &read_FDS, NULL, NULL, &tv)) == -1){
				if(errno != EINTR){ //EINTR is caused by gprof
					perror("select");
					exit(1);
				}
			}
		}

		//process reads/writes
		char recv_buff[global::C_MAX_SIZE];
		std::map<int, client_buffer *>::iterator CB_iter;
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &read_FDS)){
				int n_bytes = recv(socket_FD, recv_buff, global::C_MAX_SIZE, 0);
				if(n_bytes <= 0){
					{//begin lock scope
					boost::mutex::scoped_lock lock(CB_D_mutex);
					disconnect(socket_FD);
					}//end lock scope
				}
				else{
					{//begin lock scope
					boost::mutex::scoped_lock lock(CB_D_mutex);
					CB_iter = Client_Buffer.find(socket_FD);
					CB_iter->second->recv_buff.append(recv_buff, n_bytes);
					CB_iter->second->post_recv();
					}//end lock scope
				}
			}
			if(FD_ISSET(socket_FD, &write_FDS)){
				{//begin lock scope
				boost::mutex::scoped_lock lock(CB_D_mutex);
				CB_iter = Client_Buffer.find(socket_FD);

				if(!CB_iter->second->send_buff.empty()){
					int n_bytes = send(socket_FD, CB_iter->second->send_buff.c_str(), CB_iter->second->send_buff.size(), 0);
					if(n_bytes <= 0){
						disconnect(socket_FD);
					}
					else{ //remove bytes sent from buffer
						CB_iter->second->send_buff.erase(0, n_bytes);
						CB_iter->second->post_send();
					}
				}
				}//end lock scope
			}
		}
	}

	--threads;
}

void client::new_conn(download_conn * DC)
{
	++threads;

	//delay connection if connection to this server already in progress.
	while(true){
		bool found = false;

		{//begin lock scope
		boost::mutex::scoped_lock lock(DC_mutex);
		std::list<download_conn *>::iterator iter_cur, iter_end;
		iter_cur = Pending_Connection.begin();
		iter_end = Pending_Connection.end();
		while(iter_cur != iter_end){
			if((*iter_cur)->server_IP == DC->server_IP && (*iter_cur)->processing){
				found = true;
				break;
			}
			++iter_cur;
		}

		//proceed to trying to make a connection
		if(!found){
			DC->processing = true;
			break;
		}
		}//end lock scope

		//connection to this server in progess
		if(found){
			sleep(1);
		}
	}

	//if the socket is already connected, add the download but do not make a new connection
	int new_FD = 0;
	{//begin lock scope
	boost::mutex::scoped_lock lock(CB_D_mutex);
	for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
		if(FD_ISSET(socket_FD, &master_FDS)){
			if(DC->server_IP == Client_Buffer[socket_FD]->get_IP()){
				new_FD = socket_FD;
			}
		}
	}
	}//end lock scope

	//create new connection if no existing connection exists
	if(new_FD == 0){
		sockaddr_in dest_addr;
		new_FD = socket(PF_INET, SOCK_STREAM, 0);
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(global::P2P_PORT);
		dest_addr.sin_addr.s_addr = inet_addr(DC->server_IP.c_str());
		memset(&(dest_addr.sin_zero),'\0',8);

		if(connect(new_FD, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0){
			new_FD = 0; //this indicates a failed connection
		}
		else{
			#ifdef DEBUG
			std::cout << "info: client::new_conn() created socket " << new_FD << " for " << DC->server_IP << "\n";
			#endif

			{//begin lock scope
			boost::mutex::scoped_lock lock(CB_D_mutex);
			//make sure FD_max is always the highest socket number
			if(new_FD > FD_max){
				FD_max = new_FD;
			}
			Client_Buffer.insert(std::make_pair(new_FD, new client_buffer(new_FD, DC->server_IP, send_pending)));
			}//end lock scope

			if(!FD_ISSET(new_FD, &master_FDS)){
				FD_SET(new_FD, &master_FDS);
			}
		}
	}

	/*
	Register download connection with the Client_Buffer.
	*/
	if(new_FD != 0){
		//register this connection with the download
		DC->Download->reg_conn(new_FD, DC);

		{//begin lock scope
		boost::mutex::scoped_lock lock(CB_D_mutex);
		Client_Buffer[new_FD]->add_download(DC->Download);
		}//end lock scope
	}

	//remove the element from Pending_Connection
	{//begin lock scope
	boost::mutex::scoped_lock lock(DC_mutex);
	std::list<download_conn *>::iterator iter_cur, iter_end;
	iter_cur = Pending_Connection.begin();
	iter_end = Pending_Connection.end();
	while(iter_cur != iter_end){
		if(*iter_cur == DC){
			Pending_Connection.erase(iter_cur);
			break;
		}
		++iter_cur;
	}
	}//end lock scope

	--threads;
}

void client::prepare_requests()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(CB_D_mutex);
	std::map<int, client_buffer *>::iterator iter_cur, iter_end;
	iter_cur = Client_Buffer.begin();
	iter_end = Client_Buffer.end();
	while(iter_cur != iter_end){
		iter_cur->second->prepare_request();
		++iter_cur;
	}
	}//end lock scope
}

void client::remove_complete()
{
	//locate completed downloads
	{//begin lock scope
	boost::mutex::scoped_lock lock(CB_D_mutex);
	std::list<std::string> completeHash;
	std::list<download *>::iterator iter_cur, iter_end;
	iter_cur = Download_Buffer.begin();
	iter_end = Download_Buffer.end();
	while(iter_cur != iter_end){
		if((*iter_cur)->complete()){
			completeHash.push_back((*iter_cur)->hash());
		}
		++iter_cur;
	}

	/*
	If termination can be done in all elements that contain the download then
	remove the Download element.
	*/
	std::list<std::string>::iterator hash_iter_cur, hash_iter_end;
	hash_iter_cur = completeHash.begin();
	hash_iter_end = completeHash.end();
	while(hash_iter_cur != hash_iter_end){
		bool terminated = true;
		//attempt to terminate the download with all Client_Buffer elements
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &master_FDS)){
				if(Client_Buffer[socket_FD]->terminate_download(*hash_iter_cur)){
					if(Client_Buffer[socket_FD]->empty()){
						disconnect(socket_FD);
					}
				}
				else{
					terminated = false;
				}
			}
		}

		if(terminated){
			//termination successfull, remove the Download element
			std::list<download *>::iterator iter_cur, iter_end;
			iter_cur = Download_Buffer.begin();
			iter_end = Download_Buffer.end();
			while(iter_cur != iter_end){
				if(*hash_iter_cur == (*iter_cur)->hash()){
					Client_Index.terminate_download((*iter_cur)->hash());
					delete *iter_cur;
					Download_Buffer.erase(iter_cur);
					break;
				}
				++iter_cur;
			}

			//remove the completeHash element, termination successful
			hash_iter_cur = completeHash.erase(hash_iter_cur);
		}
		else{ //termination failed
			++hash_iter_cur;
		}
	}

	if(completeHash.empty()){
		*download_complete = false;
	}
	}//end lock scope
}

void client::start()
{
	boost::thread T1(boost::bind(&client::main_thread, this));
}

void client::stop()
{
	stop_threads = true;

	//spin-lock idea, wait for threads to terminate
	while(threads){
		usleep(100);
	}
}

bool client::start_download(exploration::info_buffer info)
{
	//make sure file isn't already downloading
	if(!info.resumed){
		if(!Client_Index.start_download(info)){
			return false;
		}
	}

	//get file path, stop if file not found
	std::string file_path;
	if(!Client_Index.get_file_path(info.hash, file_path)){
		return false;
	}

	//create an empty file for this download, if a file doesn't already exist
	std::fstream fin(file_path.c_str(), std::ios::in);
	if(fin.is_open()){
		fin.close();
	}
	else{
		std::fstream fout(file_path.c_str(), std::ios::out);
		fout.close();
	}

	unsigned long file_size = strtoul(info.file_size.c_str(), NULL, 10);
	unsigned int latest_request = info.latest_request;
	unsigned int last_block = atol(info.file_size.c_str())/(global::P_BLS_SIZE - 1); //(global::P_BLS_SIZE - 1) because control size is 1 byte
	unsigned int last_block_size = atol(info.file_size.c_str()) % (global::P_BLS_SIZE - 1) + 1;

	download * Download = new download_file(
		info.hash,
		info.file_name,
		file_path,
		file_size,
		latest_request,
		last_block,
		last_block_size,
		download_complete
	);

	{//begin lock scope
	boost::mutex::scoped_lock lock(CB_D_mutex);
	Download_Buffer.push_back(Download);
	}//end lock scope

	//queue pending connections
	std::vector<download_conn *> temp_DC;
	{//begin lock scope
	boost::mutex::scoped_lock lock(DC_mutex);
	for(int x = 0; x < info.server_IP.size(); ++x){
		download_conn * DC = new download_file_conn(Download, info.server_IP[x], atoi(info.file_ID[x].c_str()));
		Pending_Connection.push_back(DC);
		temp_DC.push_back(DC);
	}
	}//end lock scope

	//connect
	std::vector<download_conn *>::iterator iter_cur, iter_end;
	iter_cur = temp_DC.begin();
	iter_end = temp_DC.end();
	while(iter_cur != iter_end){
		boost::thread T(boost::bind(&client::new_conn, this, *iter_cur));
		++iter_cur;
	}

	return true;
}

void client::stop_download(const std::string & hash)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(CB_D_mutex);
	std::list<download *>::iterator iter_cur, iter_end;
	iter_cur = Download_Buffer.begin();
	iter_end = Download_Buffer.end();
	while(iter_cur != iter_end){
		if(hash == (*iter_cur)->hash()){
			(*iter_cur)->stop();
			break;
		}
		++iter_cur;
	}
	}//end lock scope
}

