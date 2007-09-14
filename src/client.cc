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
	FD_ZERO(&masterfds);

	sendPending = new int(0);
	downloadComplete = new bool(false);

	//reconnect downloads that havn't finished
	std::list<exploration::infoBuffer> resumedDownload;
	ClientIndex.initialFillBuffer(resumedDownload);

	for(std::list<exploration::infoBuffer>::iterator iter0 = resumedDownload.begin(); iter0 != resumedDownload.end(); iter0++){
		startDownload(*iter0);
	}
}

client::~client()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

	for(std::vector<clientBuffer *>::iterator iter0 = ClientBuffer.begin(); iter0 != ClientBuffer.end(); iter0++){
		if(*iter0 != 0){
			delete *iter0;
		}
	}

	for(std::list<download *>::iterator iter0 = DownloadBuffer.begin(); iter0 != DownloadBuffer.end(); iter0++){
		delete *iter0;
	}

	}//end lock scope

	delete sendPending;
	delete downloadComplete;
}

//this function should only be used from within a ClientDownloadBufferMutex
inline void client::disconnect(const int & socketfd)
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

	ClientBuffer[socketfd]->unregisterAll();
	delete ClientBuffer[socketfd];
	ClientBuffer[socketfd] = 0;

	//reduce ClientBuffer if possible
	while(ClientBuffer.size() > fdmax + 1){

		if(ClientBuffer.back() != 0){
			delete ClientBuffer.back();
		}

		ClientBuffer.pop_back();
	}
}

bool client::getDownloadInfo(std::vector<infoBuffer> & downloadInfo)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

	if(DownloadBuffer.size() == 0){
		return false;
	}

	for(std::list<download *>::iterator iter0 = DownloadBuffer.begin(); iter0 != DownloadBuffer.end(); iter0++){

		//calculate the percent complete
		float bytesDownloaded = (*iter0)->getLatestRequest() * (global::BUFFER_SIZE - global::S_CTRL_SIZE);
		float fileSize = (*iter0)->getFileSize();
		float percent = (bytesDownloaded / fileSize) * 100;
		int percentComplete = int(percent);

		//convert fileSize from B to MB
		fileSize = fileSize / 1024 / 1024;
		std::ostringstream fileSize_s;

		if(fileSize < 1){
			fileSize_s << std::setprecision(2) << fileSize << " mB";
		}
		else{
			fileSize_s << (int)fileSize << " mB";
		}

		//convert the download speed to kiloBytes
		int downloadSpeed = (*iter0)->getSpeed() / 1024;
		std::ostringstream downloadSpeed_s;
		downloadSpeed_s << downloadSpeed << " kB/s";

		infoBuffer info;
		info.hash = (*iter0)->getHash();

		//get all IP's
		info.server_IP.splice(info.server_IP.end(), (*iter0)->get_IPs());

		info.fileName = (*iter0)->getFileName();
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

int client::getTotalSpeed()
{
	int speed = 0;

	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

	for(std::list<download *>::iterator iter0 = DownloadBuffer.begin(); iter0 != DownloadBuffer.end(); iter0++){
		speed += (*iter0)->getSpeed();
	}

	}//end lock scope

	return speed;
}

void client::main_thread()
{
	//how long select() waits before returning(unless data received)
	timeval tv;

	//main client receive loop
	while(true){

		if(*downloadComplete){
			removeCompleted();
		}

		//trigger generation of new requests for ready sockets
		prepareRequests();

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
		for(int socketfd = 0; socketfd <= fdmax; socketfd++){
			if(FD_ISSET(socketfd, &readfds)){
				read_socket(socketfd);
			}
			if(FD_ISSET(socketfd, &writefds)){
				write_socket(socketfd);
			}
		}
	}
}

bool client::newConnection(pendingConnection * PC)
{
	//make sure the socket isn't already connected
	for(int socketfd = 0; socketfd < ClientBuffer.size(); socketfd++){
		if(ClientBuffer[socketfd] != 0){
			if(PC->server_IP == ClientBuffer[socketfd]->get_IP()){
				ClientBuffer[socketfd]->addDownload(atoi(PC->file_ID.c_str()), PC->Download);
				return true;
			}
		}
	}

	sockaddr_in dest_addr;
	int socketfd = socket(PF_INET, SOCK_STREAM, 0);

	dest_addr.sin_family = AF_INET;                               //set socket type TCP
	dest_addr.sin_port = htons(global::P2P_PORT);                 //set destination port
	dest_addr.sin_addr.s_addr = inet_addr(PC->server_IP.c_str()); //set destination IP
	memset(&(dest_addr.sin_zero),'\0',8);                         //zero out the rest

	if(connect(socketfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0){
		return false; //new connection failed
	}
	else{
		FD_SET(socketfd, &masterfds);
	}
#ifdef DEBUG
	std::cout << "info: client::newConnection() created socket " << socketfd << " for " << PC->server_IP << "\n";
#endif

	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

	//make sure fdmax is always the highest socket number
	if(socketfd > fdmax){
		fdmax = socketfd;

		//resize send/receive buffers if necessary
		while(ClientBuffer.size() <= fdmax){
			ClientBuffer.push_back((clientBuffer *)0);
		}
	}

	if(ClientBuffer[socketfd] == 0){
		ClientBuffer[socketfd] = new clientBuffer(PC->server_IP, sendPending);
	}

	ClientBuffer[socketfd]->addDownload(atoi(PC->file_ID.c_str()), PC->Download);

	}//end lock scope

	return true; //new connection created
}

void client::prepareRequests()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

	for(std::vector<clientBuffer *>::iterator iter0 = ClientBuffer.begin(); iter0 != ClientBuffer.end(); iter0++){
		if(*iter0 != 0){
			(*iter0)->prepareRequest();
		}
	}

	}//end lock scope
}

inline void client::read_socket(const int & socketfd)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

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

void client::removeCompleted()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

	//locate completed downloads
	std::list<std::string> completeHash;
	for(std::list<download *>::iterator iter0 = DownloadBuffer.begin(); iter0 != DownloadBuffer.end(); iter0++){
		if((*iter0)->complete()){
			completeHash.push_back((*iter0)->getHash());
		}
	}

	/*
	Try to terminate the download in every ClientBuffer element. If it can be done
	in all elements then remove the Download element.
	*/
	std::list<std::string>::iterator hash_iter = completeHash.begin();
	while(hash_iter != completeHash.end()){

		bool terminated = true;

		//attempt to terminate the download with all ClientBuffer elements
		for(int socketfd = 0; socketfd < ClientBuffer.size(); socketfd++){

			if(ClientBuffer[socketfd] != 0){

				if(ClientBuffer[socketfd]->terminateDownload(*hash_iter)){
					if(ClientBuffer[socketfd]->empty()){
						disconnect(socketfd);
					}
				}
				else{
					terminated = false;
				}
			}
		}

		if(terminated){ //termination successful

			//remove the Download element
			for(std::list<download *>::iterator iter0 = DownloadBuffer.begin(); iter0 != DownloadBuffer.end(); iter0++){
				if(*hash_iter == (*iter0)->getHash()){
					ClientIndex.terminateDownload((*iter0)->getHash());
					delete *iter0;
					DownloadBuffer.erase(iter0);
					break;
				}
			}

			//remove the deferredTermination element since termination was successful
			hash_iter = completeHash.erase(hash_iter);
		}
		else{ //termination failed

			hash_iter++;
		}
	}

	if(completeHash.empty()){
		*downloadComplete = false;
	}

	}//end lock scope
}

void client::start()
{
	boost::thread T1(boost::bind(&client::main_thread, this));

	for(int x = 0; x < global::TREAD_POOL_SIZE; x++){
		boost::thread T2(boost::bind(&client::serverConnect_thread, this));
	}
}

bool client::startDownload(exploration::infoBuffer info)
{
	//make sure file isn't already downloading
	if(!info.resumed){
		if(!ClientIndex.startDownload(info)){
			return false;
		}
	}

	//get file path, stop if file not found(it should be)
	std::string filePath;
	if(!ClientIndex.getFilePath(info.hash, filePath)){
		return false;
	}

	int fileSize = atoi(info.fileSize.c_str());
	int latestRequest = info.latestRequest;
	int lastBlock = atoi(info.fileSize.c_str())/(global::BUFFER_SIZE - global::S_CTRL_SIZE);
	int lastBlockSize = atoi(info.fileSize.c_str()) % (global::BUFFER_SIZE - global::S_CTRL_SIZE) + global::S_CTRL_SIZE;
	int lastSuperBlock = lastBlock / global::SUPERBLOCK_SIZE;
	int currentSuperBlock = info.currentSuperBlock;

	download * Download = new download(
		info.hash,
		info.fileName,
		filePath,
		fileSize,
		latestRequest,
		lastBlock,
		lastBlockSize,
		lastSuperBlock,
		currentSuperBlock,
		downloadComplete
	);

	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

	DownloadBuffer.push_back(Download);

	}//end lock scope

	{//begin lock scope
	boost::mutex::scoped_lock lock(PendingConnectionMutex);

	//queue up all sockets to be connected
	for(int x=0; x<info.server_IP.size(); x++){

		//add to existing container if possible
		bool found = false;
		for(std::list<std::list<pendingConnection *> >::iterator iter0 = PendingConnection.begin(); iter0 != PendingConnection.end(); iter0++){
			if(iter0->back()->server_IP == info.server_IP[x]){
				iter0->push_front(new pendingConnection(Download, info.server_IP[x], info.file_ID[x]));
				found = true;
			}
		}

		if(!found){
			std::list<pendingConnection *> temp;
			temp.push_back(new pendingConnection(Download, info.server_IP[x], info.file_ID[x]));
			PendingConnection.push_back(temp);
		}
	}

	}//end lock scope

	return true;
}

void client::serverConnect_thread()
{
	pendingConnection * PC = 0;

	while(true){

		{//begin lock scope
		boost::mutex::scoped_lock lock(PendingConnectionMutex);

		if(!PendingConnection.empty()){

			for(std::list<std::list<pendingConnection *> >::iterator iter0 = PendingConnection.begin(); iter0 != PendingConnection.end(); iter0++){
				if(iter0->back()->status == FREE){
					PC = iter0->back();
					iter0->back()->status = PROCESSING;
					break;
				}
			}
		}

		}//end lock scope

		if(PC != 0){

			newConnection(PC);

			{//begin lock scope
			boost::mutex::scoped_lock lock(PendingConnectionMutex);

			for(std::list<std::list<pendingConnection *> >::iterator iter0 = PendingConnection.begin(); iter0 != PendingConnection.end(); iter0++){
				if(iter0->back() == PC){
					delete iter0->back();
					iter0->pop_back();

					if(iter0->empty()){
						PendingConnection.erase(iter0);
					}
					break;
				}
			}

			}//end lock scope

			PC = 0;
		}
		else{
			sleep(1);
		}
	}
}

void client::stopDownload(const std::string & hash)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

	//locate completed downloads
	std::list<std::string> completeHash;
	for(std::list<download *>::iterator iter0 = DownloadBuffer.begin(); iter0 != DownloadBuffer.end(); iter0++){
		if(hash == (*iter0)->getHash()){
			(*iter0)->stop();
			break;
		}
	}

	}//end lock scope
}

inline void client::write_socket(const int & socketfd)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

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

