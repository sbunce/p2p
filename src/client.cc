#include <iostream>
#include <fstream>
#include <iomanip>
#include <list>
#include <sstream>

#include "client.h"

client::client()
{
	fdmax = 0;

	//no mutex needed because client thread can't be running yet
	ClientIndex.initialFillBuffer(sendBuffer);
}

void client::terminateDownload(std::string messageDigest_in)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(sendBufferMutex);

	//find the file and remove it from the sendBuffer
	for(std::vector<download>::iterator iter = sendBuffer.begin(); iter < sendBuffer.end(); iter++){

		if(messageDigest_in == iter->getMessageDigest()){

			//disconnect all sockets associated with the download
			for(std::list<download::serverElement>::iterator subIter = iter->Server.begin(); subIter != iter->Server.end(); subIter++){
				disconnect(subIter->socketfd);
			}

			//get rid of the download
			ClientIndex.terminateDownload(iter->getMessageDigest());
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

	//if socket = -1 then there is no corresponding socket in readfds
	if(socketfd != -1){
		close(socketfd);

		{//begin lock scope
		boost::mutex::scoped_lock lock(readfdsMutex);
		FD_CLR(socketfd, &readfds);
		}//end lock scope
	}
}

bool client::getDownloadInfo(std::vector<infoBuffer> & downloadInfo)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(sendBufferMutex);

	if(sendBuffer.size() == 0){
		return false;
	}

	//process sendQueue
	for(std::vector<download>::iterator iter = sendBuffer.begin(); iter != sendBuffer.end(); iter++){
		//calculate the percent complete
		float bytesDownloaded = iter->getBlockCount() * (global::BUFFER_SIZE - global::CONTROL_SIZE);
		float fileSize = iter->getFileSize();
		float percent = (bytesDownloaded / fileSize) * 100;
		int percentComplete = int(percent);

		//convert fileSize from Bytes to megaBytes
		fileSize = iter->getFileSize() / 1024 / 1024;
		std::ostringstream fileSize_s;

		if(fileSize < 1){
			fileSize_s << std::setprecision(2) << fileSize << " mB";
		}
		else{
			fileSize_s << (int)fileSize << " mB";
		}

		//convert the download speed to kiloBytes
		int downloadSpeed = iter->getSpeed() / 1024;
		std::ostringstream downloadSpeed_s;
		downloadSpeed_s << downloadSpeed << " kB/s";

		infoBuffer info;
		info.messageDigest = iter->getMessageDigest();

		//get all IP's
		for(std::list<download::serverElement>::iterator subIter = iter->Server.begin(); subIter != iter->Server.end(); subIter++){
			info.server_IP += subIter->server_IP;
		}

		info.fileName = iter->getFileName();
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

	for(std::vector<download>::iterator iter = sendBuffer.begin(); iter != sendBuffer.end(); iter++){
		speed += iter->getSpeed();
	}

	}//end lock scope

	speed = speed / 1024; //convert to kB
	std::ostringstream speed_s;
	speed_s << speed << " kB/s";

	return speed_s.str();
}

int client::processBuffer(int socketfd, char recvBuff[], int nbytes)
{
	for(std::vector<download>::iterator iter = sendBuffer.begin(); iter < sendBuffer.end(); iter++){
		//found the right bucket
		if(iter->hasSocket(socketfd)){

			iter->processBuffer(socketfd, recvBuff, nbytes);

			if(iter->complete()){
				terminateDownload(iter->getMessageDigest());
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
   int nbytes;                          //how many bytes received in one shot

	//how long select() waits before returning(unless data received)
	timeval tv;

	//main client receive loop
	while(true){
		sendPendingRequests();

		/*
		These must be initialized every iteration on linux because linux will change
		them(POSIX.1-2001 allows this). This work-around doesn't adversely effect
		other operating systems.
		*/
		tv.tv_sec = 1;
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
	for(std::vector<download>::iterator iter = sendBuffer.begin(); iter < sendBuffer.end(); iter++){
		for(std::list<download::serverElement>::iterator subIter = iter->Server.begin(); subIter != iter->Server.end(); subIter++){
			if(subIter->ready){

				int request = iter->getRequest();
				sendRequest(subIter->server_IP, subIter->socketfd, subIter->file_ID, request);

				//will be ready again when a response it received to this requst
				subIter->ready = false;

				/*
				If the last request was made to a server the serverElement must be marked
				so it knows to expect a incomplete packet.
				*/
				if(request == iter->getLastBlock()){
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
	//make sure file isn't already downloading
	if(ClientIndex.startDownload(info) < 0){
		return false;
	}

	/*
	This could be done with less copying but this function only gets run once when
	a download starts and this way is much more clear and self-documenting.
	*/
	std::string filePath = ClientIndex.getFilePath(info.messageDigest);
	int fileSize = atoi(info.fileSize_bytes.c_str());
	int blockCount = 0;
	int lastBlock = atoi(info.fileSize_bytes.c_str())/(global::BUFFER_SIZE - global::CONTROL_SIZE);
	int lastBlockSize = atoi(info.fileSize_bytes.c_str()) % (global::BUFFER_SIZE - global::CONTROL_SIZE) + global::CONTROL_SIZE;
	int lastSuperBlock = lastBlock / global::SUPERBLOCK_SIZE;
	int currentSuperBlock = 0;

	download newDownload(
		info.messageDigest,
		info.fileName,
		filePath,
		fileSize,
		blockCount,
		lastBlock,
		lastBlockSize,
		lastSuperBlock,
		currentSuperBlock
	);

	//add known servers associated with this download
	for(int x=0; x<info.server_IP.size(); x++){
		download::serverElement SE;
		SE.server_IP = info.server_IP.at(x);
		SE.socketfd = -1;
		SE.ready = true;
		SE.file_ID = atoi(info.file_ID.at(x).c_str());
		SE.lastRequest = false;
		newDownload.Server.push_back(SE);
	}

	{//begin lock scope
	boost::mutex::scoped_lock lock(sendBufferMutex);
	sendBuffer.push_back(newDownload);
	}//end lock scope

	return true;
}

