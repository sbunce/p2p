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
	sendPending = 0;
	FD_ZERO(&masterfds);

	ServerIndex.start();
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
	if(FD_ISSET(socketfd, &masterfds)){
		--connections;
		close(socketfd);
		FD_CLR(socketfd, &masterfds);
	}

	//reduce fdmax if possible
	for(int x=fdmax; x != 0; --x){
		if(FD_ISSET(x, &masterfds)){
			fdmax = x;
			break;
		}
	}

	{//begin lock scope
	boost::mutex::scoped_lock lock(sendBufferMutex);

	//erase any remaining buffer
	sendBuffer[socketfd].clear();

	//reduce send/receive buffers if possible
	while(sendBuffer.size() >= fdmax+1){
		sendBuffer.pop_back();
	}

	}//end lock scope

	{//begin lock scope
	boost::mutex::scoped_lock lock(receiveBufferMutex);

	//erase any remaining buffer
	receiveBuffer[socketfd].clear();

	while(receiveBuffer.size() >= fdmax+1){
		receiveBuffer.pop_back();
	}

	}//end lock scope
}

void server::calculateSpeed(const int & socketfd, const int & file_ID, const int & fileBlock)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(uploadSpeedMutex);

	time_t currentTime = time(0);

	//get IP of the socket
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getpeername(socketfd, (struct sockaddr*)&addr, &len);

	bool found = false;
	std::list<speedElement>::iterator iter_cur, iter_end;
	iter_cur = uploadSpeed.begin();
	iter_end = uploadSpeed.end();
	while(iter_cur != iter_end){

		//if element found update it
		if(iter_cur->client_IP == inet_ntoa(addr.sin_addr) && iter_cur->file_ID == file_ID){
			found = true;

			//check time and update byte count
			if(iter_cur->downloadSecond.front() == currentTime){
				iter_cur->secondBytes.front() += global::BUFFER_SIZE;
			}
			else{
				iter_cur->downloadSecond.push_front(currentTime);
				iter_cur->secondBytes.push_front(global::BUFFER_SIZE);
			}

			iter_cur->fileBlock = fileBlock;

			//get rid of elements older than SPEED_AVERAGE seconds
			//+2 on SPEED_AVERAGE because first and last second will be discarded
			if(iter_cur->downloadSecond.back() <= currentTime - (global::SPEED_AVERAGE + 2)){
				iter_cur->downloadSecond.pop_back();
				iter_cur->secondBytes.pop_back();
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
		if(ServerIndex.getFileInfo(file_ID, file_size, filePath)){

			speedElement temp;
			temp.file_name = filePath.substr(filePath.find_last_of("/")+1);
			temp.client_IP = inet_ntoa(addr.sin_addr);
			temp.file_ID = file_ID;
			temp.fileBlock = fileBlock;
			temp.downloadSecond.push_back(currentTime);
			temp.secondBytes.push_back(global::BUFFER_SIZE);
			temp.file_size = file_size;
			temp.fileBlock = fileBlock;

			uploadSpeed.push_back(temp);
		}
	}
	}//end lock scope
}

int server::get_total_speed()
{
	int totalBytes = 0;

	{//begin lock scope
	boost::mutex::scoped_lock lock(uploadSpeedMutex);
	std::list<speedElement>::iterator iter_cur, iter_end;
	iter_cur = uploadSpeed.begin();
	iter_end = uploadSpeed.end();
	while(iter_cur != iter_end){
		int DS_size = iter_cur->downloadSecond.size();
		for(int x=1; x<DS_size - 1; ++x){
			totalBytes += iter_cur->secondBytes[x];
		}
		++iter_cur;
	}
	}//end lock scope

	int speed = totalBytes / global::SPEED_AVERAGE;
	return speed;
}

bool server::getUploadInfo(std::vector<info_buffer> & uploadInfo)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(uploadSpeedMutex);

	//remove completed downloads from uploadSpeed
	time_t currentTime = time(0);
	std::list<speedElement>::iterator iter_cur, iter_end;
	iter_cur = uploadSpeed.begin();
	iter_end = uploadSpeed.end();
	while(iter_cur != iter_end){
		if(iter_cur->downloadSecond.front() < (int)currentTime - global::COMPLETE_REMOVE){
			iter_cur = uploadSpeed.erase(iter_cur);
		}
		else{
			++iter_cur;
		}
	}

	if(uploadSpeed.empty()){
		return false;
	}

	iter_cur = uploadSpeed.begin();
	iter_end = uploadSpeed.end();
	while(iter_cur != iter_end){
		float percent = ((iter_cur->fileBlock * global::BUFFER_SIZE) / (float)iter_cur->file_size) * 100;
		if(percent > 100){
			percent = 100;
		}

		//add up bytes for SPEED_AVERAGE seconds
		int totalBytes = 0;
		int DS_size = iter_cur->downloadSecond.size();
		for(int x=1; x<DS_size - 1; ++x){
			totalBytes += iter_cur->secondBytes[x];
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
	return ServerIndex.is_indexing();
}

bool server::newConnection(const int & listener)
{
	struct sockaddr_in remoteaddr;
	socklen_t len = sizeof(remoteaddr);

	//make a new socket for incoming connection
	int newfd = accept(listener, (struct sockaddr *)&remoteaddr, &len);

	//make sure the client isn't already connected
	std::string new_IP(inet_ntoa(remoteaddr.sin_addr));
	struct sockaddr_in temp_addr;
	for(int socketfd=0; socketfd<=fdmax; ++socketfd){
		if(FD_ISSET(socketfd, &masterfds)){
			getpeername(socketfd, (struct sockaddr*)&temp_addr, &len);
			if(strcmp(new_IP.c_str(), inet_ntoa(temp_addr.sin_addr)) == 0){
#ifdef DEBUG
				std::cout << "error: server::newConnection(): client " << new_IP << " attempted multiple connections\n";
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
			FD_SET(newfd, &masterfds);

			//make sure fdmax is correct, resize buffers if needed
			if(newfd > fdmax){
				fdmax = newfd;

				{//begin lock scope
				boost::mutex::scoped_lock lock(sendBufferMutex);

				//resize send/receive buffers if necessary
				while(sendBuffer.size() <= fdmax){
					std::string newBuffer;
					sendBuffer.push_back(newBuffer);
				}

				sendBuffer[newfd].reserve(global::BUFFER_SIZE);

				}//end lock scope

				{//begin lock scope
				boost::mutex::scoped_lock lock(receiveBufferMutex);

				while(receiveBuffer.size() <= fdmax){
					std::string newBuffer;
					receiveBuffer.push_back(newBuffer);
				}

				}//end lock scope
			}
#ifdef DEBUG
			std::cout << "info: server::newConnection(): " << inet_ntoa(remoteaddr.sin_addr) << " socket " << newfd << " connected\n";
#endif
		}
		else{ //too many connections, send rejection to new socket and disconnect
			char temp[] = {global::P_REJ, '\0'};
			send(newfd, temp, sizeof(temp), 0);
			close(newfd);
#ifdef DEBUG
			std::cout << "warning: server::newConnection(): max connections reached, rejected new connection\n";
#endif
		}
	}

	return true;
}

unsigned int server::decodeInt(const int & begin, char recvBuffer[])
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

	FD_SET(listener, &masterfds);
	fdmax = listener;
	char recvBuffer[global::BUFFER_SIZE];
   int nbytes;

#ifdef DEBUG
	std::cout << "info: server::start_thread(): server created listener socket number " << listener << "\n";
#endif

	//resize sendBuffer
	while(sendBuffer.size() <= fdmax){
		std::string newBuffer;
		sendBuffer.push_back(newBuffer);
	}

	while(true){
		readfds = masterfds;
		writefds = masterfds;
		/*
		This if(sendPending) exists to save a LOT of CPU time by not checking
		if sockets are ready to write when we don't need to write anything.
		*/
		if(sendPending != 0){
			if((select(fdmax+1, &readfds, &writefds, NULL, NULL)) == -1){
				//gprof will send PROF signal, this will ignore it
				if(errno != EINTR){
					perror("select");
					exit(1);
				}
			}
		}
		else{
			if((select(fdmax+1, &readfds, NULL, NULL, NULL)) == -1){
				//gprof will send PROF signal, this will ignore it
				if(errno != EINTR){
					perror("select");
					exit(1);
				}
			}
		}

		for(int socketfd=0; socketfd<=fdmax; ++socketfd){
			if(FD_ISSET(socketfd, &readfds)){
				if(socketfd == listener){ //new client connected
					newConnection(socketfd);
				}
				else{ //existing socket sending data
					if((nbytes = recv(socketfd, recvBuffer, sizeof(recvBuffer), 0)) <= 0){
						disconnect(socketfd);
						//it's possible the disconnected socket was in writefds, try to remove it
						FD_CLR(socketfd, &writefds);
					}
					else{ //incoming data from client socket
						processRequest(socketfd, recvBuffer, nbytes);
					}
				}
			}

			if(FD_ISSET(socketfd, &writefds)){
				if(!sendBuffer[socketfd].empty()){
					if((nbytes = send(socketfd, sendBuffer[socketfd].c_str(), sendBuffer[socketfd].size(), 0)) <= 0){
						disconnect(socketfd);
					}
					else{ //remove bytes sent from buffer
						sendBuffer[socketfd].erase(0, nbytes);
						if(sendBuffer[socketfd].empty()){
							--sendPending;
						}
					}
				}
			}
		}
	}
}

int server::prepareSendBuffer(const int & socketfd, const int & file_ID, const int & blockNumber)
{
	sendBuffer[socketfd].clear(); //make sure no residual in the buffer

	//get file_size/filePath that corresponds to file_ID
	int file_size;
	std::string filePath;
	if(!ServerIndex.getFileInfo(file_ID, file_size, filePath)){
		//file was not found
		sendBuffer[socketfd] += global::P_FNF;
		return 0;
	}

	//check for valid block request
	if(blockNumber*(global::BUFFER_SIZE - global::S_CTRL_SIZE) > file_size){
		//a block past the end of file was requested
		sendBuffer[socketfd] += global::P_DNE;
		return 0;
	}

	sendBuffer[socketfd] += global::P_BLS; //add control data

	std::ifstream fin(filePath.c_str());

	if(fin.is_open()){

		//seek to the fileBlock the client wants
		fin.seekg(blockNumber*(global::BUFFER_SIZE - global::S_CTRL_SIZE));

		//fill the buffer
		char ch;
		while(fin.get(ch)){
			sendBuffer[socketfd] += ch;
			if(sendBuffer[socketfd].size() == global::BUFFER_SIZE){
				break;
			}
		}

		fin.close();
	}

	//update speed calculation (assumes there is a response)
	calculateSpeed(socketfd, file_ID, blockNumber);

	return 0;
}

void server::processRequest(const int & socketfd, char recvBuffer[], const int & nbytes)
{
	if(nbytes == global::C_CTRL_SIZE){

		if(recvBuffer[0] == global::P_SBL){

			int file_ID = decodeInt(1, recvBuffer);
			int blockNumber = decodeInt(5, recvBuffer);

			prepareSendBuffer(socketfd, file_ID, blockNumber);

			++sendPending;

#ifdef DEBUG_VERBOSE
			sockaddr_in addr;
			socklen_t len = sizeof(addr);
			getpeername(socketfd, (struct sockaddr*)&addr, &len);
			std::cout << "info: server::queueRequest(): server received " << blockNumber << " from " << inet_ntoa(addr.sin_addr) << "\n";
#endif
		}
	}
	else{
		receiveBuffer[socketfd].append(recvBuffer);
	}
}

void server::start()
{
	boost::thread T(boost::bind(&server::main_thread, this));
}

