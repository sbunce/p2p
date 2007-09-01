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


//Reconnecting resumed downloads should be done here.
	//ClientIndex.initialFillBuffer(downloadBuffer, serverHolder);
/*
	if(!downloadBuffer.empty()){
		resumeDownloads = true;
	}
*/
	newDownloadPending = false;

	sendPending = new int(0);
	downloadComplete = new bool(false);
}

client::~client()
{
	for(std::vector<clientBuffer *>::iterator iter0 = ClientBuffer.begin(); iter0 != ClientBuffer.end(); iter0++){
		if(*iter0 != 0){
			delete *iter0;
		}
	}

	for(std::list<download *>::iterator iter0 = DownloadBuffer.begin(); iter0 != DownloadBuffer.end(); iter0++){
		delete *iter0;
	}

	delete sendPending;
	delete downloadComplete;
}


//only call this function from within a ClientDownloadBufferMutex
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

	//locate completed downloads
	std::list<std::string> completeHash;
	for(std::list<download *>::iterator iter0 = DownloadBuffer.begin(); iter0 != DownloadBuffer.end(); iter0++){

		//calculate the percent complete
		float bytesDownloaded = (*iter0)->getLatestRequest() * (global::BUFFER_SIZE - global::S_CTRL_SIZE);
		float fileSize = (*iter0)->getFileSize();
		float percent = (bytesDownloaded / fileSize) * 100;
		int percentComplete = int(percent);

		//convert fileSize from B to MB
		fileSize = (*iter0)->getFileSize() / 1024 / 1024;
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
		boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

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
	else{
		{//begin lock scope
		boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

		ClientBuffer[socketfd] = new clientBuffer(server_IP, sendPending);

		}//end lock scope
	}

	FD_SET(socketfd, &masterfds);
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
	boost::thread clientThread(boost::bind(&client::start_thread, this));
}

void client::start_thread()
{
	//how long select() waits before returning(unless data received)
	timeval tv;

	//main client receive loop
	while(true){

		//start user initiated downloads
		if(newDownloadPending){
			startDeferredDownloads();
		}

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

void client::startDownload(exploration::infoBuffer info)
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(scheduledDownloadMutex);

	scheduledDownload.push_back(info);

	}//end lock scope

	newDownloadPending = true;
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

	newDownloadPending = false;
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
		currentSuperBlock,
		downloadComplete
	);

	DownloadBuffer.push_back(Download);

	//add known servers associated with this download
	for(int x = 0; x < info.server_IP.size(); x++){

		bool socketFound = false;

		{//begin lock scope
		boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

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

			int socketfd;

			if(!newConnection(socketfd, info.server_IP[x])){
#ifdef DEBUG
				std::cout << "info: client::startDownload() could not connect to " << info.server_IP[x] << "\n";
#endif
				continue;
			}
			else{

				{//begin lock scope
				boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

				ClientBuffer[socketfd]->addDownload(atoi(info.file_ID[x].c_str()), Download);

				}//end lock scope
			}
		}
	}

	return true;
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

