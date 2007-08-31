//std
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iomanip>
#include <list>
#include <sstream>

#include "client.h"

client::client()
{
	fdmax = 0;
	//ClientIndex.initialFillBuffer(downloadBuffer, serverHolder);
/*
	if(!downloadBuffer.empty()){
		resumeDownloads = true;
	}
*/
	sendPending = new int(0);
	FD_ZERO(&masterfds);
}

client::~client()
{
	for(std::vector<clientBuffer *>::iterator iter0 = ClientBuffer.begin(); iter0 != ClientBuffer.end(); iter0++){
		if(*iter0 != 0){
			delete *iter0;
		}
	}

	delete sendPending;
}

void client::disconnect(const int & socketfd)
{
#ifdef DEBUG
	std::cout << "info: client disconnecting socket number " << socketfd << "\n";
#endif

	close(socketfd);
	FD_CLR(socketfd, &masterfds);

	//reduce fdmax if possible
	for(int x=fdmax; x != 0; x--){
		if(FD_ISSET(x, &masterfds)){
			fdmax = x;
			break;
		}
	}

	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientBufferMutex);

	//reduce send/receive buffers if possible
	while(ClientBuffer.size() >= fdmax){
		ClientBuffer.pop_back();
	}

	}//end lock scope
}

bool client::getDownloadInfo(std::vector<infoBuffer> & downloadInfo)
{

	return true;
}

int client::getTotalSpeed()
{

	return 69;
}

bool client::newConnection(int & socketfd, std::string & server_IP)
{
	sockaddr_in dest_addr;
	socketfd = socket(PF_INET, SOCK_STREAM, 0);

	dest_addr.sin_family = AF_INET;                           //set socket type TCP
	dest_addr.sin_port = htons(global::P2P_PORT);             //set destination port
	dest_addr.sin_addr.s_addr = inet_addr(server_IP.c_str()); //set destination IP
	memset(&(dest_addr.sin_zero),'\0',8);                     //zero out the rest

	if(connect(socketfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) == -1){
		return false; //new connection failed
	}
#ifdef DEBUG
	std::cout << "info: client::newConnection() created socket " << socketfd << " for " << server_IP << "\n";
#endif

	//make sure fdmax is always the highest socket number
	if(socketfd > fdmax){
		fdmax = socketfd;

		{//begin lock scope
		boost::mutex::scoped_lock lock(ClientBufferMutex);

		//resize send/receive buffers if necessary
		while(ClientBuffer.size() <= fdmax){

			if(ClientBuffer.size() == socketfd){
				ClientBuffer.push_back(new clientBuffer(server_IP, sendPending));
			}
			else{
				ClientBuffer.push_back((clientBuffer *)0);
			}
		}

		}//end lock scope
	}

	FD_SET(socketfd, &masterfds);
	return true; //new connection created
}

void client::postResumeConnect()
{
//	if(resumeDownloads){

/* resume those downloads
		sockaddr_in dest_addr;

		//create sockets for the resumed downloads
		for(std::list<std::list<download::serverElement *> >::iterator iter0 = serverHolder.begin(); iter0 != serverHolder.end(); iter0++){

			int socketfd;
			newConnection(socketfd, iter0->front()->server_IP);
	
			//set all server elements with this IP to the same socket
			for(std::list<download::serverElement *>::iterator iter1 = iter0->begin(); iter1 != iter0->end(); iter1++){
				(*iter1)->socketfd = socketfd;
			}
		}
*/

		resumeDownloads = false;
//	}
}

void client::prepareRequests()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientBufferMutex);

	for(std::vector<clientBuffer *>::iterator iter0 = ClientBuffer.begin(); iter0 != ClientBuffer.end(); iter0++){
		if(*iter0 != 0){
			(*iter0)->prepareRequest();
		}
	}

	}//end lock scope
}

void client::resetHungDownloadSpeed()
{
	//call zeroSpeed() on a download without any associated sockets
}

void client::removeAbusive()
{
//remove the socket if it has been marked as abusive

//	resetHungDownloadSpeed();
}

void client::removeDisconnected(const int & socketfd)
{
//remove the element associated with the socket


//	resetHungDownloadSpeed();
}

void client::removeTerminated()
{
//there should be a bool to determine if the body of this even gets run.


/*
	{//begin lock scope
	boost::mutex::scoped_lock lock(downloadBufferMutex);

	for(std::list<download>::iterator iter0 = downloadBuffer.begin(); iter0 != downloadBuffer.end(); iter0++){

		if(iter0->readyTerminate()){
#ifdef DEBUG
			std::cout << "info: client::removeTerminated() deferred termination of \"" << iter0->getFileName() << "\" done\n";
#endif

			{//begin lock scope
			boost::mutex::scoped_lock lock(deferredTerminationMutex);

			deferredTermination.push_back(iter0->getHash());

			}//end lock scope
		}
	}

	}//end lock scope

	{//begin lock scope
	boost::mutex::scoped_lock lock(deferredTerminationMutex);

	for(std::vector<std::string>::iterator iter0 = deferredTermination.begin(); iter0 != deferredTermination.end(); iter0++){
		terminateDownload(*iter0);
	}

	deferredTermination.clear();

	}//end lock scope
*/
}

void client::start()
{
	boost::thread clientThread(boost::bind(&client::start_thread, this));
}

void client::start_thread()
{

	//how long select() waits before returning(unless data received)
	timeval tv;

	//main client receive loop
	while(true){

		postResumeConnect();      //resumes downloads after program start
		prepareRequests();
		startDeferredDownloads(); //starts scheduled downloads
		removeAbusive();          //remove servers marked as abusive
		removeTerminated();       //check for downloads scheduled for deletion

		/*
		These must be initialized every iteration on linux(and possibly other OS's)
		because linux will change them(POSIX.1-2001 allows this) to reflect the
		time that select() has blocked for. This work-around doesn't adversely
		effect other operating systems.
		*/
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		/*
		This if(sendPending) exists to save a LOT of CPU time by not checking
		if sockets are ready to write when we don't need to write anything.
		*/
		if(*sendPending != 0){
			readfds = masterfds;
			writefds = masterfds;

			if((select(fdmax+1, &readfds, &writefds, NULL, &tv)) == -1){
				if(errno != EINTR){
					perror("select");
					exit(1);
				}
			}
		}
		else{
			FD_ZERO(&writefds);
			readfds = masterfds;

			if((select(fdmax+1, &readfds, NULL, NULL, &tv)) == -1){
				if(errno != EINTR){
					perror("select");
					exit(1);
				}
			}
		}

		//begin loop through all of the sockets, checking for flags
		for(int socket=0; socket <= fdmax; socket++){
			if(FD_ISSET(socket, &readfds)){
				read_socket(socket);
			}
			if(FD_ISSET(socket, &writefds)){
				write_socket(socket);
			}
		}
	}
}

inline void client::read_socket(const int & socketfd)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientBufferMutex);

	char recvBuff[global::BUFFER_SIZE];
	int nbytes = recv(socketfd, recvBuff, global::BUFFER_SIZE, 0);

	if(nbytes <= 0){
		disconnect(socketfd);
	}
	else{
		ClientBuffer[socketfd]->recvBuffer.append(recvBuff, nbytes);
		ClientBuffer[socketfd]->postRecv();
	}

	}//end lock scope
}

inline void client::write_socket(const int & socketfd)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientBufferMutex);

	if(!ClientBuffer[socketfd]->sendBuffer.empty()){

		int nbytes = send(socketfd, ClientBuffer[socketfd]->sendBuffer.c_str(), ClientBuffer[socketfd]->sendBuffer.size(), 0);

		if(nbytes <= 0){
			disconnect(socketfd);
		}
		else{ //remove bytes sent from buffer
			ClientBuffer[socketfd]->sendBuffer.erase(0, nbytes);
			ClientBuffer[socketfd]->postSend();
		}
	}

	}//end lock scope
}

void client::startDownload(exploration::infoBuffer info)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(scheduledDownloadMutex);
	scheduledDownload.push_back(info);
	}//end lock scope
}

void client::startDeferredDownloads()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(scheduledDownloadMutex);
	for(std::list<exploration::infoBuffer>::iterator iter0 = scheduledDownload.begin(); iter0 != scheduledDownload.end(); iter0++){
		startDownload_deferred(*iter0);
	}

	scheduledDownload.clear();
	}//end lock scope
}

bool client::startDownload_deferred(exploration::infoBuffer info)
{
	//make sure file isn't already downloading
	if(!ClientIndex.startDownload(info)){
		return false;
	}

	//get file path, stop if file not found(it should be)
	std::string filePath;
	if(!ClientIndex.getFilePath(info.hash, filePath)){
		return false;
	}

	int fileSize = atoi(info.fileSize.c_str());
	int blockCount = 0;
	int lastBlock = atoi(info.fileSize.c_str())/(global::BUFFER_SIZE - global::S_CTRL_SIZE);
	int lastBlockSize = atoi(info.fileSize.c_str()) % (global::BUFFER_SIZE - global::S_CTRL_SIZE) + global::S_CTRL_SIZE;
	int lastSuperBlock = lastBlock / global::SUPERBLOCK_SIZE;
	int currentSuperBlock = 0;

	download * Download = new download(
		info.hash,
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
	for(int x = 0; x < info.server_IP.size(); x++){

		bool socketFound = false;

		{//begin lock scope
		boost::mutex::scoped_lock lock(ClientBufferMutex);

		//see if a socket already exists for the IP
		for(int socket = 0; socket <= fdmax; socket++){
			if(FD_ISSET(socket, &masterfds)){
				if(ClientBuffer[socket]->get_IP() == info.server_IP[x]){
					ClientBuffer[socket]->addDownload(atoi(info.file_ID[x].c_str()), Download);
					socketFound = true;
				}
			}
		}

		}//end lock scope

		if(socketFound){
			continue;
		}

		//if existing socket not found create a new one
		if(!socketFound){

			int socket;
			if(!newConnection(socket, info.server_IP[x])){
#ifdef DEBUG
				std::cout << "info: client::startDownload() could not connect to " << info.server_IP[x] << "\n";
#endif
				continue;
			}
			else{

				{//begin lock scope
				boost::mutex::scoped_lock lock(ClientBufferMutex);

				ClientBuffer[socket]->addDownload(atoi(info.file_ID[x].c_str()), Download);

				}//end lock scope
			}
		}
	}

	return true;
}

void client::stopDownload(const std::string & hash)
{
/*
	{//begin lock scope
	boost::mutex::scoped_lock lock(deferredTerminationMutex);

	deferredTermination.push_back(hash);

	}//end lock scope
*/
}

void client::terminateDownload(const std::string & hash)
{
/*
	//this function removes downloads

	//remove download entry from the database
	ClientIndex.terminateDownload(download_iter->getHash());
*/
}

