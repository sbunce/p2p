#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "client.h"

client::client()
{
	fdmax = 0;

	//no mutex needed because client_thread can't be running yet
	ClientIndex.initialFillBuffer(sendBuffer);
}

void client::cancelDownload(std::string messageDigest_in)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(sendBufferMutex);

	//find the file and remove it from the sendBuffer
	for(std::vector<clientBuffer>::iterator iter = sendBuffer.begin(); iter < sendBuffer.end(); iter++){
		if(messageDigest_in == iter->messageDigest){

			//get rid of the download entry in the download index
			ClientIndex.downloadComplete(iter->messageDigest);
			disconnect(iter->socketfd);
			sendBuffer.erase(iter);
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
	for(int x=0; x<sendBuffer.size(); x++){
		//calculate the percent complete
		float bytesDownloaded = sendBuffer.at(x).blockCount * (global::BUFFER_SIZE - global::CONTROL_SIZE);
		float fileSize = sendBuffer.at(x).fileSize;
		float percent = (bytesDownloaded / fileSize) * 100;
		int percentComplete = int(percent);

		//convert fileSize from Bytes to megaBytes
		fileSize = sendBuffer.at(x).fileSize / 1024 / 1024;
		std::ostringstream fileSize_s;

		if(fileSize < 1){
			fileSize_s << std::setprecision(2) << fileSize << " mB";
		}
		else{
			fileSize_s << (int)fileSize << " mB";
		}

		//convert the download speed to kiloBytes
		int downloadSpeed = sendBuffer.at(x).downloadSpeed / 1024;
		std::ostringstream downloadSpeed_s;
		downloadSpeed_s << downloadSpeed << " kB/s";

		std::ostringstream file_ID_s;
		file_ID_s << sendBuffer.at(x).file_ID;

		infoBuffer info;
		info.messageDigest = sendBuffer.at(x).messageDigest;
		info.IP = sendBuffer.at(x).server_IP;
		info.file_ID = file_ID_s.str();
		info.fileName = sendBuffer.at(x).fileName;
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

			//fill the bucket
			std::string received(recvBuff, nbytes);
			iter->bucket += received;

#ifdef DEBUG_VERBOSE
			std::cout << "info: bucket.size(): " << iter->bucket.size() << std::endl;
#endif

			//disconnect the server if it's being nasty!
			if(iter->bucket.size() > 3*global::BUFFER_SIZE){
#ifdef DEBUG
				std::cout << "error: client::processBuffer() detected buffer overrun from " << iter->server_IP << std::endl;
#endif
				disconnect(iter->socketfd);
				removeCompleted(iter->messageDigest);
			}

			//if full block received write to tree and prepare another request
			if(iter->bucket.size() % global::BUFFER_SIZE == 0){
				iter->addToTree(iter->bucket);
				iter->ready = true;
			}

			/*
			Check for completed download. The (blockCount - 1) is because blockCount
			is incremented after every request, making the final block request when
			(blockCount - 1) == lastBlock.
			*/
			if(iter->blockCount - 1 == iter->lastBlock){

				//check for zero in case the last block was exactly BUFFER_SIZE
				if(iter->bucket.size() != 0){
					//add last partial block to tree
					if(iter->bucket.size() == iter->lastBlockSize){
						iter->addToTree(iter->bucket);
std::cout << "added last block" << std::endl;
					}
					else{ //full block not yet received
						break;
					}
				}

				if(iter->completed()){
					removeCompleted(iter->messageDigest);
				}
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

void client::removeCompleted(std::string messageDigest)
{
	//just in case something unexpected happens and we're told to remove from an empty buffer
	if(!sendBuffer.empty()){

		for(std::vector<clientBuffer>::iterator iter = sendBuffer.begin(); iter < sendBuffer.end(); iter++){

			if(iter->messageDigest == messageDigest){
				//get rid of the download entry in the download index
				ClientIndex.downloadComplete(iter->messageDigest);

				//disconnect from server
				disconnect(iter->socketfd);

				sendBuffer.erase(iter);

				break;
			}
		}
	}
	else{
#ifdef DEBUG
		std::cout << "error: client::removeCompleted was called when send buffer empty" << std::endl;
#endif
	}
}

void client::sendPendingRequests()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(sendBufferMutex);

	//process sendQueue
	for(std::vector<clientBuffer>::iterator iter = sendBuffer.begin(); iter < sendBuffer.end(); iter++){
		if(iter->ready){
			//send request
			sendRequest(iter->server_IP, iter->socketfd, iter->file_ID, iter->getBlockRequestNumber());

			iter->ready = false;
			iter->blockCount++;
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

bool client::startDownload(std::string messageDigest, std::string server_IP, std::string file_ID, std::string fileName, std::string fileSize)
{
	if(ClientIndex.startDownload(messageDigest, server_IP, file_ID, fileSize, fileName) < 0){
		return false;
	}

	//fill bufferSubElement
	clientBuffer temp_cb;
	temp_cb.messageDigest = messageDigest;
	temp_cb.server_IP = server_IP;
	temp_cb.socketfd = -1;
	temp_cb.fileName = fileName;
	temp_cb.filePath = ClientIndex.getFilePath(messageDigest);
	temp_cb.file_ID = atoi(file_ID.c_str());
	temp_cb.blockCount = 0;
	temp_cb.fileSize = atoi(fileSize.c_str());
	temp_cb.lastBlock = atoi(fileSize.c_str())/(global::BUFFER_SIZE - global::CONTROL_SIZE);
	temp_cb.lastBlockSize = temp_cb.fileSize % (global::BUFFER_SIZE - global::CONTROL_SIZE) + global::CONTROL_SIZE;

	{//begin lock scope
	boost::mutex::scoped_lock lock(sendBufferMutex);
	sendBuffer.push_back(temp_cb);
	}//end lock scope

	return true;
}

