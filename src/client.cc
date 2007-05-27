#include <iostream>
#include <fstream>
#include <iomanip>
#include <list>
#include <sstream>

#include "client.h"

client::client()
{
	fdmax = 0;

	//no mutex needed because client_thread can't be running yet
	ClientIndex.initialFillBuffer(sendBuffer);
}

void client::terminateDownload(std::string messageDigest_in)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(sendBufferMutex);

	//find the file and remove it from the sendBuffer
	for(std::vector<clientBuffer>::iterator iter = sendBuffer.begin(); iter < sendBuffer.end(); iter++){

		if(messageDigest_in == iter->messageDigest){

			//disconnect all sockets associated with the download
			for(std::list<clientBuffer::serverElement>::iterator subIter = iter->Server.begin(); subIter != iter->Server.end(); subIter++){
				disconnect(subIter->socketfd);
			}

			//get rid of the clientBuffer
			sendBuffer.erase(iter);
			ClientIndex.downloadComplete(iter->messageDigest);
		}
	}

	}//end lock scope
}

void client::disconnect(int socketfd)
{
#ifdef DEBUG
	std::cout << "info: client disconnecting socket number " << socketfd << std::endl;
#endif

	close(socketfd);

	{//begin lock scope
	boost::mutex::scoped_lock lock(readfdsMutex);
	FD_CLR(socketfd, &readfds);
	}//end lock scope
}

bool client::getDownloadInfo(std::vector<infoBuffer> & downloadInfo)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(sendBufferMutex);

	if(sendBuffer.size() == 0){
		return false;
	}

	//process sendQueue
	for(std::vector<clientBuffer>::iterator iter = sendBuffer.begin(); iter != sendBuffer.end(); iter++){
		//calculate the percent complete
		float bytesDownloaded = iter->blockCount * (global::BUFFER_SIZE - global::CONTROL_SIZE);
		float fileSize = iter->fileSize;
		float percent = (bytesDownloaded / fileSize) * 100;
		int percentComplete = int(percent);

		//convert fileSize from Bytes to megaBytes
		fileSize = iter->fileSize / 1024 / 1024;
		std::ostringstream fileSize_s;

		if(fileSize < 1){
			fileSize_s << std::setprecision(2) << fileSize << " mB";
		}
		else{
			fileSize_s << (int)fileSize << " mB";
		}

		//convert the download speed to kiloBytes
		int downloadSpeed = iter->downloadSpeed / 1024;
		std::ostringstream downloadSpeed_s;
		downloadSpeed_s << downloadSpeed << " kB/s";

		infoBuffer info;
		info.messageDigest = iter->messageDigest;

		//get all IP's
		for(std::list<clientBuffer::serverElement>::iterator subIter = iter->Server.begin(); subIter != iter->Server.end(); subIter++){
			info.IP += subIter->server_IP;
		}

		info.fileName = iter->fileName;
		info.fileSize = fileSize_s.str();
		info.speed = downloadSpeed_s.str();
		info.percentComplete = percentComplete;

		//happens sometimes during testing or if something went wrong
		if(percentComplete > 100){
			info.percentComplete = 100;
		}

		downloadInfo.push_back(info);
	}

	}//end lock scope

	return true;
}

std::string client::getTotalSpeed()
{
	int speed = 0;

	{//begin lock scope
	boost::mutex::scoped_lock lock(sendBufferMutex);

	for(int x=0; x<sendBuffer.size(); x++){
		speed += sendBuffer.at(x).downloadSpeed;
	}

	}//end lock scope

	speed = speed / 1024; //convert to kB
	std::ostringstream speed_s;
	speed_s << speed << " kB/s";

	return speed_s.str();
}

int client::processBuffer(int socketfd, char recvBuff[], int nbytes)
{
	for(std::vector<clientBuffer>::iterator iter = sendBuffer.begin(); iter < sendBuffer.end(); iter++){
		//found the right bucket
		if(iter->hasSocket(socketfd)){

			iter->processBuffer(socketfd, recvBuff, nbytes);

			if(iter->complete()){
				terminateDownload(iter->messageDigest);
			}

			break;
		}
	}
}

void client::start()
{
	boost::thread clientThread(boost::bind(&client::start_thread, this));
}

void client::start_thread()
{
	char recvBuff[global::BUFFER_SIZE];  //the receive buffer
   int nbytes;                  //how many bytes received in one shot

	//used to get the address of the server
	sockaddr_in addr;
	socklen_t len = sizeof(addr);

	//main client receive loop
	while(true){
		sendPendingRequests();

		/*
		These must be initialized every iteration on linux because linux will change
		them(POSIX.1-2001 allows this). This work-around doesn't adversely effect
		other operating systems.
		*/
		timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		{//begin lock scope
		boost::mutex::scoped_lock lock(readfdsMutex);
		if((select(fdmax+1, &readfds, NULL, NULL, &tv)) == -1){
			perror("select");
			exit(1);
		}
		}//end lock scope

		//begin loop through all of the sockets, checking for flags
		for(int x=0; x<=fdmax; x++){
			if(FD_ISSET(x, &readfds)){
				if((nbytes = recv(x, recvBuff, sizeof(recvBuff), 0)) <= 0){
					disconnect(x);
				}
				else{
					getpeername(x, (struct sockaddr*)&addr, &len);
					processBuffer(x, recvBuff, nbytes);
				}
			}
		}
	}
}

void client::sendPendingRequests()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(sendBufferMutex);

	//process sendQueue
	for(std::vector<clientBuffer>::iterator iter = sendBuffer.begin(); iter < sendBuffer.end(); iter++){
		for(std::list<clientBuffer::serverElement>::iterator subIter = iter->Server.begin(); subIter != iter->Server.end(); subIter++){
			if(subIter->ready){

				int request = iter->getRequest();
				sendRequest(subIter->server_IP, subIter->socketfd, subIter->file_ID, request);
				subIter->ready = false;
				if(request == iter->lastBlock){
					subIter->lastRequest = true;
				}
			}
		}
	}

	}//end lock scope
}

int client::sendRequest(std::string server_IP, int & socketfd, int file_ID, int fileBlock)
{
	bool newConnection = false; 
	sockaddr_in dest_addr;

	//if no socket then create one
	if(socketfd == -1){
		socketfd = socket(PF_INET, SOCK_STREAM, 0);
		FD_SET(socketfd, &readfds);
		newConnection = true;
	}

	dest_addr.sin_family = AF_INET;                           //set socket type TCP
	dest_addr.sin_port = htons(global::P2P_PORT);             //set destination port
	dest_addr.sin_addr.s_addr = inet_addr(server_IP.c_str()); //set destination IP
	memset(&(dest_addr.sin_zero),'\0',8);                     //zero out the rest of struct

	//make sure fdmax is always the highest socket number
	if(socketfd > fdmax){
		fdmax = socketfd;
	}

	if(newConnection){
		if(connect(socketfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) == -1){
#ifdef DEBUG
			std::cout << "error: client::sendRequest() could not connect to server" << std::endl;
#endif
			return 0;
		}
	}

	std::ostringstream sendRequest_s;
	sendRequest_s << global::P_SBL << "," << file_ID << "," << fileBlock << ",";

	int bytesToSend = sendRequest_s.str().length();
	int nbytes = 0; //how many bytes sent in one shot

	//send request
	while(bytesToSend > 0){
		nbytes = send(socketfd, sendRequest_s.str().c_str(), sendRequest_s.str().length(), 0);

		if(nbytes == -1){
#ifdef DEBUG
			std::cout << "error: client::sendRequest() had error on sending request" << std::endl;
#endif
			break;
		}

		bytesToSend -= nbytes;
	}

#ifdef DEBUG_VERBOSE
	std::cout << "info: client sending " << sendRequest_s.str() << " to " << server_IP << std::endl;
#endif

	return 0;
}

bool client::startDownload(exploration::infoBuffer info)
{
	if(ClientIndex.startDownload(info) < 0){
		return false;
	}

	//fill bufferSubElement
	clientBuffer temp_cb;
	temp_cb.messageDigest = info.messageDigest;
	temp_cb.fileName = info.fileName;
	temp_cb.filePath = ClientIndex.getFilePath(info.messageDigest);
	temp_cb.blockCount = 0;
	temp_cb.fileSize = atoi(info.fileSize_bytes.c_str());
	temp_cb.lastBlock = atoi(info.fileSize_bytes.c_str())/(global::BUFFER_SIZE - global::CONTROL_SIZE);
	temp_cb.lastBlockSize = atoi(info.fileSize_bytes.c_str()) % (global::BUFFER_SIZE - global::CONTROL_SIZE) + global::CONTROL_SIZE;
	temp_cb.lastSuperBlock = temp_cb.lastBlock / global::SUPERBLOCK_SIZE;

	for(int x=0; x<info.IP.size(); x++){
		clientBuffer::serverElement temp_se;
		temp_se.server_IP = info.IP.at(x);
		temp_se.socketfd = -1;
		temp_se.ready = true;
		temp_se.file_ID = atoi(info.file_ID.at(x).c_str());
		temp_se.lastRequest = false;

		temp_cb.Server.push_back(temp_se);
	}

	{//begin lock scope
	boost::mutex::scoped_lock lock(sendBufferMutex);
	sendBuffer.push_back(temp_cb);
	}//end lock scope

	return true;
}

