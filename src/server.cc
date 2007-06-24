//std
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <time.h>

#include "server.h"

server::server()
{
	maxConnections = global::MAX_CONNECTIONS;
	numConnections = 0;

	indexing = true;
}

void server::disconnect(int clientSock)
{
#ifdef DEBUG
	std::cout << "info: server::disconnect(): server disconnecting socket number " << clientSock << "\n";
#endif
	numConnections--;

	close(clientSock);
	FD_CLR(clientSock, &masterfds);
}

void server::calculateSpeed(int clientSock, int file_ID, int fileBlock)
{
	time_t currentTime = time(0);

	//get IP of the socket
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getpeername(clientSock, (struct sockaddr*)&addr, &len);

	bool found = false;
	for(std::vector<speedElement>::iterator iter0 = uploadSpeed.begin(); iter0 != uploadSpeed.end(); iter0++){

		//if element found update it
		if(iter0->client_IP == inet_ntoa(addr.sin_addr) && iter0->file_ID == file_ID){
			found = true;

			//check time and update byte count
			if(iter0->downloadSecond.front() == currentTime){
				iter0->secondBytes.front() += global::BUFFER_SIZE;
			}
			else{
				iter0->downloadSecond.push_front(currentTime);
				iter0->secondBytes.push_front(global::BUFFER_SIZE);
			}

			iter0->fileBlock = fileBlock;

			//get rid of elements older than SPEED_AVERAGE seconds
			//+2 on SPEED_AVERAGE because first and last second will be discarded
			if(iter0->downloadSecond.back() <= currentTime - (global::SPEED_AVERAGE + 2)){
				iter0->downloadSecond.pop_back();
				iter0->secondBytes.pop_back();
			}

			break;
		}
	}

	//make a new element if there is no speed element for this client
	if(!found){
		int fileSize;
		std::string filePath;
		std::string fileName;

		//only add an element if we have the file requested
		if(ServerIndex.getFileInfo(file_ID, fileSize, filePath)){

			speedElement temp;
			temp.fileName = filePath.substr(filePath.find_last_of("/")+1);
			temp.client_IP = inet_ntoa(addr.sin_addr);
			temp.file_ID = file_ID;
			temp.fileBlock = fileBlock;
			temp.downloadSecond.push_back(currentTime);
			temp.secondBytes.push_back(global::BUFFER_SIZE);
			temp.fileSize = fileSize;
			temp.fileBlock = fileBlock;

			uploadSpeed.push_back(temp);
		}
	}
}

std::string server::getTotalSpeed()
{
	int totalBytes = 0;

	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);

	/*
	Loop through all downloads to determine how many bits were downloaded in the
	last SPEED_AVERAGE seconds.
	*/
	for(std::vector<speedElement>::iterator iter0 = uploadSpeed.begin(); iter0 < uploadSpeed.end(); iter0++){
		for(int x=1; x < iter0->downloadSecond.size() - 1; x++){
			totalBytes += iter0->secondBytes[x];
		}
	}

	}//end lock scope

	int speed = totalBytes / global::SPEED_AVERAGE;

	speed = speed / 1024; //convert to kB
	std::ostringstream speed_s;
	speed_s << speed << " kB/s";

	return speed_s.str();
}

bool server::getUploadInfo(std::vector<infoBuffer> & uploadInfo)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);

	//remove completed downloads from uploadSpeed
	time_t currentTime = time(0);
	for(std::vector<speedElement>::iterator iter0 = uploadSpeed.begin(); iter0 < uploadSpeed.end(); iter0++){
		if(iter0->downloadSecond.front() < (int)currentTime - global::COMPLETE_REMOVE){
			uploadSpeed.erase(iter0);
			break;
		}
	}

	if(uploadSpeed.size() == 0){
		return false;
	}

	//process sendQueue
	for(std::vector<speedElement>::iterator iter0 = uploadSpeed.begin(); iter0 < uploadSpeed.end(); iter0++){
		//calculate the percent complete
		float bytesUploaded = iter0->fileBlock * global::BUFFER_SIZE;
		float fileSize_f = iter0->fileSize;
		float percent = (bytesUploaded / fileSize_f) * 100;
		int percentComplete;

		//floating point will never be exactly 100, this completes
		if(percent > 99){
			percentComplete = 100;
		}
		else{
			percentComplete = int(percent);
		}

		//convert fileSize from Bytes to megaBytes
		float fileSize = iter0->fileSize / 1024 / 1024;

		std::ostringstream fileSize_s;

		//change display to a decimal if less than 1mB
		if(fileSize < 1){
			fileSize_s << std::setprecision(2) << fileSize << " mB";
		}
		else{
			fileSize_s << (int)fileSize << " mB";
		}

		//add up bytes for SPEED_AVERAGE seconds
		int totalBytes = 0;
		for(int x=1; x < iter0->downloadSecond.size() - 1; x++){
			totalBytes += iter0->secondBytes[x];
		}

		//take average
		totalBytes /= global::SPEED_AVERAGE;

		//convert the download speed to kB
		int speed = totalBytes / 1024;
		std::ostringstream speed_s;
		speed_s << speed << " kB/s";

		infoBuffer info;
		info.client_IP = iter0->client_IP;
		info.file_ID = iter0->file_ID;
		info.fileName = iter0->fileName;
		info.fileSize = fileSize_s.str();
		info.speed = speed_s.str();
		info.percentComplete = percentComplete;

		uploadInfo.push_back(info);
	}

	}//end lock scope

	return true;
}

const bool & server::isIndexing()
{
	return indexing;
}

void server::newCon(int listener)
{
	struct sockaddr_in remoteaddr;
	socklen_t addrlen = sizeof(remoteaddr);
	int newfd;

	//make a new socket for incoming connection
	if((newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen)) == -1){
		perror("accept");
	}
	else{ //add new socket to master set
		numConnections++;

		if(numConnections <= maxConnections){
			FD_SET(newfd, &masterfds);

			//make sure fdmax is always the highest connected socket number,
			//if we have a low socket reused we don't want to set it as highest
			if(newfd > fdmax){
				fdmax = newfd;
			}

#ifdef DEBUG
			std::cout << "info: server::newCon(): " << inet_ntoa(remoteaddr.sin_addr) << " socket " << newfd << " connected\n";
#endif
		}
		else{ //too many connections, send rejection to new socket and disconnect
			send(newfd, global::P_REJ.c_str(), global::P_REJ.length(), 0);
			close(newfd);

#ifdef DEBUG
			std::cout << "warning: server::newCon(): max connections reached, rejected new connection\n";
#endif
		}
	}
}

void server::queueRequest(int clientSock, char recvBuff[], int nbytes)
{
	std::string request(recvBuff, nbytes);

	sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getpeername(clientSock, (struct sockaddr*)&addr, &len);

#ifdef DEBUG_VERBOSE
	std::cout << "info: server::queueRequest(): server received " << request << " from " << inet_ntoa(addr.sin_addr) << "\n";
#endif

	//if a full request was received then process it, otherwise buffer
	if(nbytes == global::REQUEST_CONTROL_SIZE){

		//find position of delimiters
		int first = request.find_first_of(global::DELIMITER);
		int second = request.find_first_of(global::DELIMITER, first+1);
		int third = request.find_first_of(global::DELIMITER, second+1);

		//tokenize control data
		std::string command = request.substr(0, first);
		std::string file_ID = request.substr(first+1, (second - 1) - first);
		std::string fileBlock = request.substr(second+1, (third - 1) - second);

		if(command == global::P_SBL){
			sendBufferElement temp;
			temp.client_IP = inet_ntoa(addr.sin_addr);
			temp.clientSock = clientSock;
			temp.file_ID = atoi(file_ID.c_str());
			temp.fileBlock = atoi(fileBlock.c_str());

			{//begin lock scope
			boost::mutex::scoped_lock lock(Mutex);

			sendBuffer.push_back(temp);

			}//end lock scope
		}
	}
	else{

		/*
		This part shouldn't run very often. It will only run if the MTU of the client
		or server is less than global::REQUEST_CONTROL_SIZE which is generally going
		to be very small.
		*/

		bool found = false;
		for(std::vector<requestBufferElement>::iterator iter0 = requestBuffer.begin(); iter0 != requestBuffer.end(); iter0++){

			if(iter0->clientSock == clientSock){
				iter0->bucket.append(recvBuff, nbytes);

				if(iter0->bucket.size() == global::REQUEST_CONTROL_SIZE){

					//find position of delimiters
					int first = request.find_first_of(global::DELIMITER);
					int second = request.find_first_of(global::DELIMITER, first+1);
					int third = request.find_first_of(global::DELIMITER, second+1);

					//tokenize control data
					std::string command = request.substr(0, first);
					std::string file_ID = request.substr(first+1, (second - 1) - first);
					std::string fileBlock = request.substr(second+1, (third - 1) - second);

					if(command == global::P_SBL){
						sendBufferElement temp;
						temp.client_IP = inet_ntoa(addr.sin_addr);
						temp.clientSock = clientSock;
						temp.file_ID = atoi(file_ID.c_str());
						temp.fileBlock = atoi(fileBlock.c_str());

						{//begin lock scope
						boost::mutex::scoped_lock lock(Mutex);

						sendBuffer.push_back(temp);

						}//end lock scope
					}
				}
				else if(iter0->bucket.size() > global::REQUEST_CONTROL_SIZE){
					//overflow detected, erase buffer
					requestBuffer.erase(iter0);
				}

				break;
			}
		}

		time_t currentTime = time(0);

		if(!found){
			requestBufferElement temp;
			temp.lastSeen = currentTime;
			temp.clientSock = clientSock;
			temp.bucket.append(recvBuff, nbytes);

			//check for overflow, add to requestBuffer if not overflow
			if(temp.bucket.size() <= global::REQUEST_CONTROL_SIZE){
				requestBuffer.push_back(temp);
			}
		}

		//remove requestBuffer elements that are too old
		for(std::vector<requestBufferElement>::iterator iter0 = requestBuffer.begin(); iter0 != requestBuffer.end(); iter0++){

			if(currentTime - iter0->lastSeen > global::TIMEOUT){
				requestBuffer.erase(iter0++); //increment the iterator while still valid
			}
		}
	}
}

int server::sendBlock(int clientSock, int file_ID, int fileBlock)
{
   int bytesToSend = 0;    //how many bytes left to send
   int nbytes;             //how many bytes sent in one shot
	std::string response;

	//check for valid file request
	int fileSize;
	std::string filePath;
	if(!ServerIndex.getFileInfo(file_ID, fileSize, filePath)){
		return 0;
	}

	//check for valid block request
	if(fileBlock*(global::BUFFER_SIZE - global::RESPONSE_CONTROL_SIZE) > fileSize){
		return 0;
	}

	//prepare control data
	response += global::P_BLS;
	bytesToSend = response.size();

	std::ifstream fin(filePath.c_str());

	if(fin.is_open()){

		//seek to the fileBlock the client wants
		fin.seekg(fileBlock*(global::BUFFER_SIZE - global::RESPONSE_CONTROL_SIZE));

		//fill the buffer
		char ch;
		while(fin.get(ch)){
			response += ch;
			bytesToSend++;
			if(bytesToSend == global::BUFFER_SIZE){
				break;
			}
		}

		fin.close();

		//send the buffer
		while(bytesToSend > 0){
			nbytes = send(clientSock, response.c_str(), bytesToSend, 0);
			if(nbytes == -1){
				break;
			}
			bytesToSend -= nbytes;
		}
	}
	else{
#ifdef DEBUG
		std::cout << "error: server::sendBlock(): can't open requested file for reading\n";
#endif
	}
}

void server::start()
{
	boost::thread serverThread(boost::bind(&server::start_thread, this));
}

void server::start_thread()
{
	/*
	The server will not start until indexing is done.
	This can potentially take a long time to calculate hashes.
	*/
	ServerIndex.indexShare();
	indexing = false;

	//set listening socket
	int listener;
	if((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(1);
	}

	//reuse port if already in use
	//if we don't do this it can take up to a minute for kernel to de-allocate port
	int yes = 1;
	if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
		perror("setsockopt");
		exit(1);
	}

	struct sockaddr_in myaddr;           //server address
	myaddr.sin_family = AF_INET;         //set ipv4
	myaddr.sin_addr.s_addr = INADDR_ANY; //set to listen on all available interfaces
	myaddr.sin_port = htons(global::P2P_PORT);   //set listening port
	memset(&(myaddr.sin_zero), '\0', 8); //zero the rest

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

	FD_SET(listener, &masterfds);   //add listener to master set
	fdmax = listener;
	char recvBuff[global::BUFFER_SIZE];     //our receive buffer
   int nbytes;                     //how many bytes sent in one shot

#ifdef DEBUG
	std::cout << "info: server::start_thread(): server created listener socket number " << listener << "\n";
#endif
	//main server loop
	while(true){
		//required because select modifies readfds
		readfds = masterfds;

		stateTick();

		if(select(fdmax+1, &readfds, NULL, NULL, NULL) == -1){
			perror("select");
			exit(1);
		}

		//begin loop through all of our sockets, checking for flags
		for(int x=0; x<=fdmax; x++){
			if(FD_ISSET(x, &readfds)){
				if(x == listener){ //new client connected
					newCon(x);
				}
				else{ //existing socket sending data
					if((nbytes = recv(x, recvBuff, sizeof(recvBuff), 0)) <= 0){
						disconnect(x);
					}
					else{ //incoming data from client socket
						queueRequest(x, recvBuff, nbytes);
					}
				}
			}
		}
	}
}

void server::stateTick()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);

	//send whole sendBuffer
	while(!sendBuffer.empty()){
		sendBlock(sendBuffer.back().clientSock, sendBuffer.back().file_ID, sendBuffer.back().fileBlock);
		calculateSpeed(sendBuffer.back().clientSock, sendBuffer.back().file_ID, sendBuffer.back().fileBlock);
		sendBuffer.pop_back();
	}

	}//end lock scope
}
