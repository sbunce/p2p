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
			delete Client_Buffer[socket_FD];
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

//this function should only be used from within a CB_D_mutex.
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

	delete Client_Buffer[socket_FD];
	Client_Buffer.erase(socket_FD);
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
		float bytesDownloaded = (*iter_cur)->get_latest_request() * (global::BUFFER_SIZE - global::S_CTRL_SIZE);
		int file_size = (*iter_cur)->get_file_size();
		float percent = (bytesDownloaded / file_size) * 100;
		int percent_complete = int(percent);
		info_buffer info;
		info.hash = (*iter_cur)->get_hash();
		info.server_IP.splice(info.server_IP.end(), (*iter_cur)->get_IPs());
		info.file_name = (*iter_cur)->get_file_name();
		info.file_size = file_size;
		if(percent_complete > 100){
			info.percent_complete = 100;
		}
		else{
			info.percent_complete = percent_complete;
		}
		info.speed = (*iter_cur)->get_speed();
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
		speed += (*iter_cur)->get_speed();
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
		char recv_buff[global::BUFFER_SIZE];
		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &read_FDS)){
				int nbytes = recv(socket_FD, recv_buff, global::BUFFER_SIZE, 0);
				if(nbytes <= 0){
					{//begin lock scope
					boost::mutex::scoped_lock lock(CB_D_mutex);
					disconnect(socket_FD);
					}//end lock scope
				}
				else{
					{//begin lock scope
					boost::mutex::scoped_lock lock(CB_D_mutex);
					Client_Buffer[socket_FD]->recv_buff.append(recv_buff, nbytes);
					Client_Buffer[socket_FD]->post_recv();
					}//end lock scope
				}
			}
			if(FD_ISSET(socket_FD, &write_FDS)){
				{//begin lock scope
				boost::mutex::scoped_lock lock(CB_D_mutex);
				if(!Client_Buffer[socket_FD]->send_buff.empty()){
					int nbytes = send(socket_FD, Client_Buffer[socket_FD]->send_buff.c_str(), Client_Buffer[socket_FD]->send_buff.size(), 0);
					if(nbytes <= 0){
						disconnect(socket_FD);
					}
					else{ //remove bytes sent from buffer
						Client_Buffer[socket_FD]->send_buff.erase(0, nbytes);
						Client_Buffer[socket_FD]->post_send();
					}
				}
				}//end lock scope
			}
		}
	}

	--threads;
}

void client::new_conn(pending_connection * PC)
{
	++threads;

	//delay connection if connection to this server already in progress.
	while(true){
		bool found = false;

		{//begin lock scope
		boost::mutex::scoped_lock lock(PC_mutex);
		std::list<pending_connection *>::iterator iter_cur, iter_end;
		iter_cur = Pending_Connection.begin();
		iter_end = Pending_Connection.end();
		while(iter_cur != iter_end){
			if((*iter_cur)->server_IP == PC->server_IP && (*iter_cur)->processing){
				found = true;
				break;
			}
			++iter_cur;
		}

		//proceed to trying to make a connection
		if(!found){
			PC->processing = true;
			break;
		}
		}//end lock scope

		//connection to this server in progess
		if(found){
			sleep(1);
		}
	}

	//if the socket is already connected, add the download but do not make a new connection
	int new_socket_FD = 0;
	{//begin lock scope
	boost::mutex::scoped_lock lock(CB_D_mutex);
	for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
		if(FD_ISSET(socket_FD, &master_FDS)){
			if(PC->server_IP == Client_Buffer[socket_FD]->get_IP()){
				new_socket_FD = socket_FD;
			}
		}
	}
	}//end lock scope

	//create new connection if no existing connection exists
	if(new_socket_FD == 0){
		sockaddr_in dest_addr;
		new_socket_FD = socket(PF_INET, SOCK_STREAM, 0);
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(global::P2P_PORT);
		dest_addr.sin_addr.s_addr = inet_addr(PC->server_IP.c_str());
		memset(&(dest_addr.sin_zero),'\0',8);

		if(connect(new_socket_FD, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0){
			new_socket_FD = 0; //this indicates a failed connection
		}
		else{
			#ifdef DEBUG
			std::cout << "info: client::new_conn() created socket " << socket_FD << " for " << PC->server_IP << "\n";
			#endif
			FD_SET(new_socket_FD, &master_FDS);

			{//begin lock scope
			boost::mutex::scoped_lock lock(CB_D_mutex);
			//make sure FD_max is always the highest socket number
			if(new_socket_FD > FD_max){
				FD_max = new_socket_FD;
			}

			Client_Buffer.insert(std::make_pair(new_socket_FD, new client_buffer(PC->server_IP, send_pending)));
			}//end lock scope
		}
	}

	//add download to socket, whether is is a new socket or an existing one
	if(new_socket_FD != 0){
		{//begin lock scope
		boost::mutex::scoped_lock lock(CB_D_mutex);
		Client_Buffer[new_socket_FD]->add_download(atoi(PC->file_ID.c_str()), PC->Download);
		}//end lock scope
	}

	//remove the element from Pending_Connection
	{//begin lock scope
	boost::mutex::scoped_lock lock(PC_mutex);
	std::list<pending_connection *>::iterator iter_cur, iter_end;
	iter_cur = Pending_Connection.begin();
	iter_end = Pending_Connection.end();
	while(iter_cur != iter_end){
		if(*iter_cur == PC){
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
			completeHash.push_back((*iter_cur)->get_hash());
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
				if(*hash_iter_cur == (*iter_cur)->get_hash()){
					Client_Index.terminate_download((*iter_cur)->get_hash());
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
	std::string filePath;
	if(!Client_Index.get_file_path(info.hash, filePath)){
		return false;
	}

	//create an empty file for this download
	std::fstream fout(filePath.c_str(), std::ios::out);
	fout.close();

	int file_size = atoi(info.file_size.c_str());
	int latest_request = info.latest_request;
	int lastBlock = atoi(info.file_size.c_str())/(global::BUFFER_SIZE - global::S_CTRL_SIZE);
	int lastBlockSize = atoi(info.file_size.c_str()) % (global::BUFFER_SIZE - global::S_CTRL_SIZE) + global::S_CTRL_SIZE;
	int lastSuperBlock = lastBlock / global::SUPERBLOCK_SIZE;
	int current_super_block = info.current_super_block;

	download * Download = new download_file(
		info.hash,
		info.file_name,
		filePath,
		file_size,
		latest_request,
		lastBlock,
		lastBlockSize,
		lastSuperBlock,
		current_super_block,
		download_complete
	);

	{//begin lock scope
	boost::mutex::scoped_lock lock(CB_D_mutex);
	Download_Buffer.push_back(Download);
	}//end lock scope

	//queue pending connections
	std::vector<pending_connection *> temp_PC;
	{//begin lock scope
	boost::mutex::scoped_lock lock(PC_mutex);
	for(int x = 0; x < info.server_IP.size(); ++x){
		pending_connection * PC = new pending_connection(Download, info.server_IP[x], info.file_ID[x]);
		Pending_Connection.push_back(PC);
		temp_PC.push_back(PC);
	}
	}//end lock scope

	//connect
	std::vector<pending_connection *>::iterator iter_cur, iter_end;
	iter_cur = temp_PC.begin();
	iter_end = temp_PC.end();
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
		if(hash == (*iter_cur)->get_hash()){
			(*iter_cur)->stop();
			break;
		}
		++iter_cur;
	}
	}//end lock scope
}

