//std
#include <fstream>
#include <iostream>
#include <time.h>

#include "conversion.h"
#include "server.h"

server::server()
{
	connections = 0;
	send_pending = 0;
	FD_ZERO(&master_FDS);
	Server_Index.start();
}

void server::disconnect(const int & socket_FD)
{
#ifdef DEBUG
	std::cout << "info: server::disconnect(): server disconnecting socket number " << socket_FD << "\n";
#endif

	/*
	It is possible for disconnect() to get called more than once when a socket
	disconnects which necessitates this check before removing the socket from the
	set and decrementing connections.
	*/
	if(FD_ISSET(socket_FD, &master_FDS)){
		--connections;
		close(socket_FD);
		FD_CLR(socket_FD, &master_FDS);
	}

	//reduce FD_max if possible
	for(int x=FD_max; x != 0; --x){
		if(FD_ISSET(x, &master_FDS)){
			FD_max = x;
			break;
		}
	}

	{//begin lock scope
	boost::mutex::scoped_lock lock(SB_mutex);
	Send_Buff.erase(socket_FD);
	}//end lock scope

	{//begin lock scope
	boost::mutex::scoped_lock lock(RB_mutex);
	Recv_Buff.erase(socket_FD);
	}//end lock scope
}

void server::calculate_speed(const int & socket_FD, const int & file_ID, const int & file_block)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(US_mutex);

	time_t currentTime = time(0);

	//get IP of the socket
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getpeername(socket_FD, (struct sockaddr*)&addr, &len);

	bool found = false;
	std::list<speed_element>::iterator iter_cur, iter_end;
	iter_cur = Upload_Speed.begin();
	iter_end = Upload_Speed.end();
	while(iter_cur != iter_end){

		//if element found update it
		if(iter_cur->client_IP == inet_ntoa(addr.sin_addr) && iter_cur->file_ID == file_ID){
			found = true;

			//check time and update byte count
			if(iter_cur->download_second.front() == currentTime){
				iter_cur->second_bytes.front() += global::P_BLS_SIZE;
			}
			else{
				iter_cur->download_second.push_front(currentTime);
				iter_cur->second_bytes.push_front(global::P_BLS_SIZE);
			}

			iter_cur->file_block = file_block;

			//get rid of elements older than SPEED_AVERAGE seconds
			//+2 on SPEED_AVERAGE because first and last second will be discarded
			if(iter_cur->download_second.back() <= currentTime - (global::SPEED_AVERAGE + 2)){
				iter_cur->download_second.pop_back();
				iter_cur->second_bytes.pop_back();
			}

			break;
		}

		++iter_cur;
	}

	//make a new element if there is no speed element for this client
	if(!found){
		int file_size;
		std::string filePath;
		std::string file_name;

		//only add an element if we have the file requested
		if(Server_Index.file_info(file_ID, file_size, filePath)){

			speed_element temp;
			temp.file_name = filePath.substr(filePath.find_last_of("/")+1);
			temp.client_IP = inet_ntoa(addr.sin_addr);
			temp.file_ID = file_ID;
			temp.file_block = file_block;
			temp.download_second.push_back(currentTime);
			temp.second_bytes.push_back(global::P_BLS_SIZE);
			temp.file_size = file_size;
			temp.file_block = file_block;

			Upload_Speed.push_back(temp);
		}
	}
	}//end lock scope
}

int server::get_total_speed()
{
	int totalBytes = 0;

	{//begin lock scope
	boost::mutex::scoped_lock lock(US_mutex);
	std::list<speed_element>::iterator iter_cur, iter_end;
	iter_cur = Upload_Speed.begin();
	iter_end = Upload_Speed.end();
	while(iter_cur != iter_end){
		int DS_size = iter_cur->download_second.size();
		for(int x=1; x<DS_size - 1; ++x){
			totalBytes += iter_cur->second_bytes[x];
		}
		++iter_cur;
	}
	}//end lock scope

	int speed = totalBytes / global::SPEED_AVERAGE;
	return speed;
}

bool server::get_upload_info(std::vector<info_buffer> & uploadInfo)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(US_mutex);

	//remove completed downloads from Upload_Speed
	time_t current_time = time(0);
	std::list<speed_element>::iterator iter_cur, iter_end;
	iter_cur = Upload_Speed.begin();
	iter_end = Upload_Speed.end();
	while(iter_cur != iter_end){
		if(iter_cur->download_second.front() < (int)current_time - global::COMPLETE_REMOVE){
			iter_cur = Upload_Speed.erase(iter_cur);
		}
		else{
			++iter_cur;
		}
	}

	if(Upload_Speed.empty()){
		return false;
	}

	iter_cur = Upload_Speed.begin();
	iter_end = Upload_Speed.end();
	while(iter_cur != iter_end){
		float percent = ((iter_cur->file_block * global::P_BLS_SIZE) / (float)iter_cur->file_size) * 100;
		if(percent > 100){
			percent = 100;
		}

		//add up bytes for SPEED_AVERAGE seconds
		int totalBytes = 0;
		int DS_size = iter_cur->download_second.size();
		for(int x=1; x<DS_size - 1; ++x){
			totalBytes += iter_cur->second_bytes[x];
		}

		//take average
		totalBytes = totalBytes / global::SPEED_AVERAGE;

		info_buffer info;
		info.client_IP = iter_cur->client_IP;
		info.file_ID = iter_cur->file_ID;
		info.file_name = iter_cur->file_name;
		info.file_size = iter_cur->file_size;
		info.speed = totalBytes;
		info.percent_complete = (int)percent;

		uploadInfo.push_back(info);
		++iter_cur;
	}
	}//end lock scope

	return true;
}

bool server::is_indexing()
{
	return Server_Index.is_indexing();
}

bool server::new_conn(const int & listener)
{
	struct sockaddr_in remoteaddr;
	socklen_t len = sizeof(remoteaddr);

	//make a new socket for incoming connection
	int new_FD = accept(listener, (struct sockaddr *)&remoteaddr, &len);

	if(new_FD == -1){
		perror("accept");
		return false;
	}

	//make sure the client isn't already connected
	std::string new_IP(inet_ntoa(remoteaddr.sin_addr));
	sockaddr_in temp_addr;
	for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
		if(FD_ISSET(socket_FD, &master_FDS)){
			getpeername(socket_FD, (struct sockaddr*)&temp_addr, &len);
			if(strcmp(new_IP.c_str(), inet_ntoa(temp_addr.sin_addr)) == 0){
#ifdef DEBUG
				std::cout << "error: server::new_conn(): client " << new_IP << " attempted multiple connections\n";
#endif
				close(new_FD);
				return false;
			}
		}
	}

	if(connections <= global::MAX_CONNECTIONS){
		++connections;

		{//begin lock scope
		boost::mutex::scoped_lock lock(SB_mutex);
		Send_Buff.insert(std::make_pair(new_FD, std::string()));
		}//end lock scope

		{//begin lock scope
		boost::mutex::scoped_lock lock(RB_mutex);
		Recv_Buff.insert(std::make_pair(new_FD, std::string()));
		}//end lock scope

		FD_SET(new_FD, &master_FDS);

		//make sure FD_max is correct, resize buffers if needed
		if(new_FD > FD_max){
			FD_max = new_FD;
		}

#ifdef DEBUG
		std::cout << "info: server::new_conn(): " << inet_ntoa(remoteaddr.sin_addr) << " socket " << new_FD << " connected\n";
#endif
	}

	return true;
}

void server::main_thread()
{
	//set listening socket
	int listener;
	if((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(1);
	}

	//reuse port if already in use
	//if this isn't done it can take up to a minute for port to be de-allocated
	int yes = 1;
	if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
		perror("setsockopt");
		exit(1);
	}

	struct sockaddr_in myaddr;                 //server address
	myaddr.sin_family = AF_INET;               //set ipv4
	myaddr.sin_addr.s_addr = INADDR_ANY;       //set to listen on all available interfaces
	myaddr.sin_port = htons(global::P2P_PORT); //set listening port
	memset(&(myaddr.sin_zero), '\0', 8);

	//set listener info to what we set above
	if(bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1){
		perror("bind");
		exit(1);
	}

	//start listener socket listening
	if(listen(listener, 10) == -1){
		perror("listen");
		exit(1);
	}

	FD_SET(listener, &master_FDS);
	FD_max = listener;
	char recv_buff[global::S_MAX_SIZE];
   int nbytes;

#ifdef DEBUG
	std::cout << "info: server::start_thread(): server created listener socket number " << listener << "\n";
#endif

	while(true){
		read_FDS = master_FDS;
		write_FDS = master_FDS;
		/*
		This if(send_pending) exists to save a LOT of CPU time by not checking
		if sockets are ready to write when we don't need to write anything.
		*/
		if(send_pending != 0){
			if((select(FD_max+1, &read_FDS, &write_FDS, NULL, NULL)) == -1){
				//gprof will send PROF signal, this will ignore it
				if(errno != EINTR){
					perror("select");
					exit(1);
				}
			}
		}
		else{
			if((select(FD_max+1, &read_FDS, NULL, NULL, NULL)) == -1){
				//gprof will send PROF signal, this will ignore it
				if(errno != EINTR){
					perror("select");
					exit(1);
				}
			}
		}

		for(int socket_FD = 0; socket_FD <= FD_max; ++socket_FD){
			if(FD_ISSET(socket_FD, &read_FDS)){
				if(socket_FD == listener){ //new client connected
					new_conn(socket_FD);
				}
				else{ //existing socket sending data
					if((nbytes = recv(socket_FD, recv_buff, global::S_MAX_SIZE, 0)) <= 0){
						disconnect(socket_FD);
						//it's possible the disconnected socket was in write_FDS, try to remove it
						FD_CLR(socket_FD, &write_FDS);
					}
					else{ //incoming data from client socket
						process_request(socket_FD, recv_buff, nbytes);
					}
				}
			}

			//do not check for writes on listener, there is no corresponding Send_Buff element (would cause segfault)
			if(FD_ISSET(socket_FD, &write_FDS) && socket_FD != listener){
				std::map<int, std::string>::iterator SB_iter = Send_Buff.find(socket_FD);

				if(!SB_iter->second.empty()){
					if((nbytes = send(socket_FD, SB_iter->second.c_str(), SB_iter->second.size(), 0)) <= 0){
						disconnect(socket_FD);
					}
					else{ //remove bytes sent from buffer
						SB_iter->second.erase(0, nbytes);
						if(SB_iter->second.empty()){
							--send_pending;
						}
					}
				}
			}
		}
	}
}

int server::prepare_file_block(std::map<int, std::string>::iterator & SB_iter, const int & socket_FD, const int & file_ID, const int & blockNumber)
{
	//make sure no residual in the buffer
	SB_iter->second.clear();

	//get file_size/filePath that corresponds to file_ID
	int file_size;
	std::string filePath;
	if(!Server_Index.file_info(file_ID, file_size, filePath)){
		//file was not found
		SB_iter->second += global::P_FNF;
		return 0;
	}

	//check for valid block request ((global::BUFFER_SIZE - 1) because control size is 1 byte)
	if(blockNumber*(global::P_BLS_SIZE - 1) > file_size){
		//a block past the end of file was requested
		SB_iter->second += global::P_DNE;
		return 0;
	}

	SB_iter->second += global::P_BLS; //add control data
	std::ifstream fin(filePath.c_str());
	if(fin.is_open()){
		//seek to the file_block the client wants
		fin.seekg(blockNumber*(global::P_BLS_SIZE - 1));

		//fill the buffer
		char ch;
		while(fin.get(ch)){
			SB_iter->second += ch;
			if(SB_iter->second.size() == global::P_BLS_SIZE){
				break;
			}
		}
		fin.close();
	}

	//update speed calculation (assumes there is a response)
	calculate_speed(socket_FD, file_ID, blockNumber);

	return 0;
}

void server::process_request(const int & socket_FD, char recv_buff[], const int & nbytes)
{
	std::map<int, std::string>::iterator SB_iter = Send_Buff.find(socket_FD);

	if(recv_buff[0] == global::P_SBL && nbytes == global::P_SBL_SIZE){
		int file_ID = conversion::decode_int(1, recv_buff);
		int blockNumber = conversion::decode_int(5, recv_buff);
		prepare_file_block(SB_iter, socket_FD, file_ID, blockNumber);
		++send_pending;
	}
	else{
		SB_iter->second.append(recv_buff);
	}
}

void server::start()
{
	boost::thread T(boost::bind(&server::main_thread, this));
}

