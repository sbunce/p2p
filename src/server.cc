//std
#include <bitset>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <time.h>

#include "server.h"

server::server()
{
	connections = 0;
	send_pending = 0;
	FD_ZERO(&master_FDS);

	Server_Index.start();
}

void server::disconnect(const int & socketfd)
{
#ifdef DEBUG
	std::cout << "info: server::disconnect(): server disconnecting socket number " << socketfd << "\n";
#endif

	/*
	It is possible for disconnect() to get called more than once when a socket
	disconnects which necessitates this check before removing the socket from the
	set and decrementing connections.
	*/
	if(FD_ISSET(socketfd, &master_FDS)){
		--connections;
		close(socketfd);
		FD_CLR(socketfd, &master_FDS);
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

	//erase any remaining buffer
	Send_Buffer[socketfd].clear();

	//reduce send/receive buffers if possible
	while(Send_Buffer.size() >= FD_max+1){
		Send_Buffer.pop_back();
	}

	}//end lock scope

	{//begin lock scope
	boost::mutex::scoped_lock lock(RB_mutex);

	//erase any remaining buffer
	Recv_Buffer[socketfd].clear();

	while(Recv_Buffer.size() >= FD_max+1){
		Recv_Buffer.pop_back();
	}

	}//end lock scope
}

void server::calculate_speed(const int & socketfd, const int & file_ID, const int & file_block)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(US_mutex);

	time_t currentTime = time(0);

	//get IP of the socket
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getpeername(socketfd, (struct sockaddr*)&addr, &len);

	bool found = false;
	std::list<speedElement>::iterator iter_cur, iter_end;
	iter_cur = Upload_Speed.begin();
	iter_end = Upload_Speed.end();
	while(iter_cur != iter_end){

		//if element found update it
		if(iter_cur->client_IP == inet_ntoa(addr.sin_addr) && iter_cur->file_ID == file_ID){
			found = true;

			//check time and update byte count
			if(iter_cur->download_second.front() == currentTime){
				iter_cur->second_bytes.front() += global::BUFFER_SIZE;
			}
			else{
				iter_cur->download_second.push_front(currentTime);
				iter_cur->second_bytes.push_front(global::BUFFER_SIZE);
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
		if(Server_Index.getFileInfo(file_ID, file_size, filePath)){

			speedElement temp;
			temp.file_name = filePath.substr(filePath.find_last_of("/")+1);
			temp.client_IP = inet_ntoa(addr.sin_addr);
			temp.file_ID = file_ID;
			temp.file_block = file_block;
			temp.download_second.push_back(currentTime);
			temp.second_bytes.push_back(global::BUFFER_SIZE);
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
	std::list<speedElement>::iterator iter_cur, iter_end;
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
	time_t currentTime = time(0);
	std::list<speedElement>::iterator iter_cur, iter_end;
	iter_cur = Upload_Speed.begin();
	iter_end = Upload_Speed.end();
	while(iter_cur != iter_end){
		if(iter_cur->download_second.front() < (int)currentTime - global::COMPLETE_REMOVE){
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
		float percent = ((iter_cur->file_block * global::BUFFER_SIZE) / (float)iter_cur->file_size) * 100;
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
	int newfd = accept(listener, (struct sockaddr *)&remoteaddr, &len);

	//make sure the client isn't already connected
	std::string new_IP(inet_ntoa(remoteaddr.sin_addr));
	struct sockaddr_in temp_addr;
	for(int socketfd=0; socketfd<=FD_max; ++socketfd){
		if(FD_ISSET(socketfd, &master_FDS)){
			getpeername(socketfd, (struct sockaddr*)&temp_addr, &len);
			if(strcmp(new_IP.c_str(), inet_ntoa(temp_addr.sin_addr)) == 0){
#ifdef DEBUG
				std::cout << "error: server::new_conn(): client " << new_IP << " attempted multiple connections\n";
#endif
				close(newfd);
				return false;
			}
		}
	}

	if(newfd == -1){
		perror("accept");
	}
	else{ //add new socket to master set
		++connections;

		if(connections <= global::MAX_CONNECTIONS){
			FD_SET(newfd, &master_FDS);

			//make sure FD_max is correct, resize buffers if needed
			if(newfd > FD_max){
				FD_max = newfd;

				{//begin lock scope
				boost::mutex::scoped_lock lock(SB_mutex);

				//resize send/receive buffers if necessary
				while(Send_Buffer.size() <= FD_max){
					std::string newBuffer;
					Send_Buffer.push_back(newBuffer);
				}

				Send_Buffer[newfd].reserve(global::BUFFER_SIZE);

				}//end lock scope

				{//begin lock scope
				boost::mutex::scoped_lock lock(RB_mutex);

				while(Recv_Buffer.size() <= FD_max){
					std::string newBuffer;
					Recv_Buffer.push_back(newBuffer);
				}

				}//end lock scope
			}
#ifdef DEBUG
			std::cout << "info: server::new_conn(): " << inet_ntoa(remoteaddr.sin_addr) << " socket " << newfd << " connected\n";
#endif
		}
		else{ //too many connections, send rejection to new socket and disconnect
			char temp[] = {global::P_REJ, '\0'};
			send(newfd, temp, sizeof(temp), 0);
			close(newfd);
#ifdef DEBUG
			std::cout << "warning: server::new_conn(): max connections reached, rejected new connection\n";
#endif
		}
	}

	return true;
}

unsigned int server::decode_int(const int & begin, char recvBuffer[])
{
	std::bitset<32> bs(0);
	std::bitset<32> bs_temp;

	int y = 3;
	for(int x=begin; x<begin+4; ++x){
		bs_temp = (unsigned char)recvBuffer[x];
		bs_temp <<= 8*y--;
		bs |= bs_temp;
	}

	return (unsigned int)bs.to_ulong();
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
	char recvBuffer[global::BUFFER_SIZE];
   int nbytes;

#ifdef DEBUG
	std::cout << "info: server::start_thread(): server created listener socket number " << listener << "\n";
#endif

	//resize Send_Buffer
	while(Send_Buffer.size() <= FD_max){
		std::string newBuffer;
		Send_Buffer.push_back(newBuffer);
	}

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

		for(int socketfd=0; socketfd<=FD_max; ++socketfd){
			if(FD_ISSET(socketfd, &read_FDS)){
				if(socketfd == listener){ //new client connected
					new_conn(socketfd);
				}
				else{ //existing socket sending data
					if((nbytes = recv(socketfd, recvBuffer, sizeof(recvBuffer), 0)) <= 0){
						disconnect(socketfd);
						//it's possible the disconnected socket was in write_FDS, try to remove it
						FD_CLR(socketfd, &write_FDS);
					}
					else{ //incoming data from client socket
						process_request(socketfd, recvBuffer, nbytes);
					}
				}
			}

			if(FD_ISSET(socketfd, &write_FDS)){
				if(!Send_Buffer[socketfd].empty()){
					if((nbytes = send(socketfd, Send_Buffer[socketfd].c_str(), Send_Buffer[socketfd].size(), 0)) <= 0){
						disconnect(socketfd);
					}
					else{ //remove bytes sent from buffer
						Send_Buffer[socketfd].erase(0, nbytes);
						if(Send_Buffer[socketfd].empty()){
							--send_pending;
						}
					}
				}
			}
		}
	}
}

int server::prepare_send_buffer(const int & socketfd, const int & file_ID, const int & blockNumber)
{
	Send_Buffer[socketfd].clear(); //make sure no residual in the buffer

	//get file_size/filePath that corresponds to file_ID
	int file_size;
	std::string filePath;
	if(!Server_Index.getFileInfo(file_ID, file_size, filePath)){
		//file was not found
		Send_Buffer[socketfd] += global::P_FNF;
		return 0;
	}

	//check for valid block request
	if(blockNumber*(global::BUFFER_SIZE - global::S_CTRL_SIZE) > file_size){
		//a block past the end of file was requested
		Send_Buffer[socketfd] += global::P_DNE;
		return 0;
	}

	Send_Buffer[socketfd] += global::P_BLS; //add control data

	std::ifstream fin(filePath.c_str());

	if(fin.is_open()){

		//seek to the file_block the client wants
		fin.seekg(blockNumber*(global::BUFFER_SIZE - global::S_CTRL_SIZE));

		//fill the buffer
		char ch;
		while(fin.get(ch)){
			Send_Buffer[socketfd] += ch;
			if(Send_Buffer[socketfd].size() == global::BUFFER_SIZE){
				break;
			}
		}

		fin.close();
	}

	//update speed calculation (assumes there is a response)
	calculate_speed(socketfd, file_ID, blockNumber);

	return 0;
}

void server::process_request(const int & socketfd, char recvBuffer[], const int & nbytes)
{
	if(nbytes == global::C_CTRL_SIZE){

		if(recvBuffer[0] == global::P_SBL){

			int file_ID = decode_int(1, recvBuffer);
			int blockNumber = decode_int(5, recvBuffer);

			prepare_send_buffer(socketfd, file_ID, blockNumber);

			++send_pending;

#ifdef DEBUG_VERBOSE
			sockaddr_in addr;
			socklen_t len = sizeof(addr);
			getpeername(socketfd, (struct sockaddr*)&addr, &len);
			std::cout << "info: server::queueRequest(): server received " << blockNumber << " from " << inet_ntoa(addr.sin_addr) << "\n";
#endif
		}
	}
	else{
		Recv_Buffer[socketfd].append(recvBuffer);
	}
}

void server::start()
{
	boost::thread T(boost::bind(&server::main_thread, this));
}

