//std
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

void server::calculateSpeed()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);

	time_t curr = time(0);
	std::ostringstream currentTime_s;
	currentTime_s << curr;
	int currentTime = atoi(currentTime_s.str().c_str());

	for(int x=0; x<sendBuffer.size(); x++){
		bool foundItem = false;

		for(int y=0; y<uploadSpeed.size(); y++){
			if(uploadSpeed.at(y).client_IP == sendBuffer.at(x).client_IP && uploadSpeed.at(y).file_ID == sendBuffer.at(x).file_ID){

				foundItem = true;

				//update the element found
				uploadSpeed.at(y).fileBlock = sendBuffer.at(x).fileBlock;
				uploadSpeed.at(y).lastSeen = currentTime;
			}
		}

		//add uploadSpeed element based on sendBuffer element if no existing entry
		if(!foundItem){
			//get IP of the socket
			struct sockaddr_in addr;
			socklen_t len = sizeof(addr);
			getpeername(sendBuffer.at(x).clientSock, (struct sockaddr*)&addr, &len);

			int fileSize;
			std::string fileName;
			std::string filePath;

			//get filePath and fileSize, abort if file is not found
			if(ServerIndex.getFileInfo(sendBuffer.at(x).file_ID, fileSize, filePath)){
				fileName = filePath.substr(filePath.find_last_of("/")+1);

				//prepare speedElement
				speedElement temp;
				temp.client_IP = inet_ntoa(addr.sin_addr);
				temp.fileName = fileName;
				temp.file_ID = sendBuffer.at(x).file_ID;
				temp.fileSize = fileSize;
				temp.fileBlock = sendBuffer.at(x).fileBlock;
				temp.lastSeen = currentTime;
				temp.lastCalc = currentTime;
				temp.oldFileBlock = sendBuffer.at(x).fileBlock;
				temp.uploadSpeed = 0;

				uploadSpeed.push_back(temp);
			}
		}
	}

	//get rid of speedElement for files that havn't been seen in sendBuffer for 5 seconds
	for(int x=0; x<uploadSpeed.size(); x++){
		if(currentTime - uploadSpeed.at(x).lastSeen > 5){
			uploadSpeed.erase(uploadSpeed.begin() + x);
		}
	}

	//calculate the upload speed if a second or more has elapsed on lastCalc
	for(int x=0; x<uploadSpeed.size(); x++){
		if(uploadSpeed.at(x).lastCalc < currentTime){
			int timeDifference = currentTime - uploadSpeed.at(x).lastCalc;
			uploadSpeed.at(x).lastCalc = currentTime;
			uploadSpeed.at(x).uploadSpeed = ((uploadSpeed.at(x).fileBlock - uploadSpeed.at(x).oldFileBlock) * global::BUFFER_SIZE) / timeDifference;
			uploadSpeed.at(x).oldFileBlock = uploadSpeed.at(x).fileBlock;
		}
	}

	}//end lock scope
}

std::string server::getTotalSpeed()
{
	//used so that calculateSpeed() gets polled occasionally,
	//regardless of whether or not there is anything to be served
	calculateSpeed();

	int speed = 0;

	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);

	for(int x=0; x<uploadSpeed.size(); x++){
		speed += uploadSpeed.at(x).uploadSpeed;
	}

	}//end lock scope

	speed = speed / 1024; //convert to kB

	std::ostringstream speed_s;
	speed_s << speed << " kB/s";

	return speed_s.str();
}

bool server::getUploadInfo(std::vector<infoBuffer> & uploadInfo)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(Mutex);

	if(uploadSpeed.size() == 0){
		return false;
	}

	//process sendQueue
	for(int x=0; x<uploadSpeed.size(); x++){
		//calculate the percent complete
		float bytesUploaded = uploadSpeed.at(x).fileBlock * (global::BUFFER_SIZE - global::CONTROL_SIZE);
		float fileSize_f = uploadSpeed.at(x).fileSize;
		float percent = (bytesUploaded / fileSize_f) * 100;
		int percentComplete;

		//floating point value will never be 100, this compensates
		if(percent > 99){
			percentComplete = 100;
		}
		else{
			percentComplete = int(percent);
		}

		//convert fileSize from Bytes to megaBytes
		float fileSize = uploadSpeed.at(x).fileSize / 1024 / 1024;

		std::ostringstream fileSize_s;

		if(fileSize < 1){
			fileSize_s << std::setprecision(2) << fileSize << " mB";
		}
		else{
			fileSize_s << (int)fileSize << " mB";
		}

		//convert the download speed to kiloBytes
		int speed = uploadSpeed.at(x).uploadSpeed / 1024;
		std::ostringstream speed_s;
		speed_s << speed << " kB/s";

		infoBuffer info;

		info.client_IP = uploadSpeed.at(x).client_IP;
		info.file_ID = uploadSpeed.at(x).file_ID;
		info.fileName = uploadSpeed.at(x).fileName;
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

	/*
	part1 - the command
	part2 - dependant on command
		if part1 == p_sendBlock then part2 == file_ID
		if part1 == p_sendBlock then part3 == fileBlock
	*/
	std::string part1 = request.substr(0, request.find_first_of(","));
	request = request.substr(request.find_first_of(",")+1);
	std::string part2 = request.substr(0, request.find_first_of(","));
	request = request.substr(request.find_first_of(",")+1);
	std::string part3 = request.substr(0, request.find_first_of(","));

	if(part1 == global::P_SBL){
		bufferElement temp;
		temp.client_IP = inet_ntoa(addr.sin_addr);
		temp.clientSock = clientSock;
		temp.file_ID = atoi(part2.c_str());
		temp.fileBlock = atoi(part3.c_str());

		{//begin lock scope
		boost::mutex::scoped_lock lock(Mutex);

		sendBuffer.push_back(temp);

		}//end lock scope
	}
	else{
		int bytesToSend = global::P_BCM.size();
		int nbytes = 0;
		while(bytesToSend > 0){
			nbytes = send(clientSock, global::P_BCM.c_str(), global::P_BCM.length(), 0);

			if(nbytes == -1){
				break;
			}

			bytesToSend -= nbytes;
		}
	}
}

int server::sendBlock(int clientSock, int file_ID, int fileBlock)
{
   int bytesToSend = 0;          //how many bytes left to send
   int nbytes;                   //how many bytes sent in one shot
	char ch;                      //single char read from stream
	char sendBuffer[global::BUFFER_SIZE]; //send buffer

	//check for valid file request
	int fileSize;
	std::string filePath;
	//true if file requested that we don't have
	if(!ServerIndex.getFileInfo(file_ID, fileSize, filePath)){
		std::string control = global::P_FNF;
		control.append(global::CONTROL_SIZE - control.length(), ' '); //blank the extra space
		control.copy(sendBuffer, global::CONTROL_SIZE);

		while(bytesToSend > 0){ //send until buffer is empty
			nbytes = send(clientSock, sendBuffer, bytesToSend, 0);

			if(nbytes == -1){
				break;
			}

			bytesToSend -= nbytes;
		}

		return 0;
	}

	//check for valid block request
	if(fileBlock*(global::BUFFER_SIZE - global::CONTROL_SIZE) > fileSize){
		std::string control = global::P_DNE;
		control.append(global::CONTROL_SIZE - control.length(), ' '); //blank the extra space
		control.copy(sendBuffer, global::CONTROL_SIZE);

		while(bytesToSend > 0){ //send until buffer is empty
			nbytes = send(clientSock, sendBuffer, bytesToSend, 0);

			if(nbytes == -1){
				break;
			}

			bytesToSend -= nbytes;
		}

		return 0;
	}

	//valid block and file requested, continue to serve request
	//prepare control data
	std::ostringstream control_s;
	control_s << global::P_BLS << "," << file_ID << "," << fileBlock << ",";
	std::string control = control_s.str();

#ifdef DEBUG_VERBOSE
	sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getpeername(clientSock, (struct sockaddr*)&addr, &len);
	std::cout << "info: server::sendBlock(): server sending " << control << " to " << inet_ntoa(addr.sin_addr) << "\n";
#endif

	control.append(global::CONTROL_SIZE - control.length(), ' '); //blank the extra space

	//copy control data to send buffer
	control.copy(sendBuffer, global::CONTROL_SIZE);
	bytesToSend = global::CONTROL_SIZE;

	std::ifstream fin(filePath.c_str());

	//block is located at the buffer size multiplied by the block number
	fin.seekg(fileBlock*(global::BUFFER_SIZE - global::CONTROL_SIZE));

	if(fin.is_open()){ //make sure the file is open
		/*
		Needed for disconnects, if client disconnects we don't want to try to send
		a partial packet to a disconnected socket
		*/
		bool packetSent = false;

		while(fin.get(ch)){ //read in one char at a time until buffer is full
			sendBuffer[bytesToSend++] = ch;

			if(bytesToSend == global::BUFFER_SIZE){ //buffer is full, send it
				while(bytesToSend > 0){      //send until buffer is empty
					nbytes = send(clientSock, sendBuffer, bytesToSend, 0);

					if(nbytes == -1){
						break;
					}

					bytesToSend -= nbytes;
				}
				packetSent = true;

				break;
			}
		}
		//EOF was hit, send what we have
		while(bytesToSend > 0 && !packetSent){
			nbytes = send(clientSock, sendBuffer, bytesToSend, 0);

			if(nbytes == -1){
				break;
			}

			bytesToSend -= nbytes;
		}

		fin.close();
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
					//check for disconnect
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
	calculateSpeed();

	//send whole sendBuffer
	while(!sendBuffer.empty()){
		sendBlock(sendBuffer.back().clientSock, sendBuffer.back().file_ID, sendBuffer.back().fileBlock);

		{//begin lock scope
		boost::mutex::scoped_lock lock(Mutex);
		sendBuffer.pop_back();
		}//end lock scope
	}
}
