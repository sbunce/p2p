//std
#include <iostream>
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

	for(std::list<exploration::infoBuffer>::iterator iter0 = resumedDownload.begin(); iter0 != resumedDownload.end(); ++iter0){
		startDownload(*iter0);
	}

	currentTime = time(0);
	previousIntervalEnd = time(0);
}

client::~client()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);
	for(std::vector<clientBuffer *>::iterator iter0 = ClientBuffer.begin(); iter0 != ClientBuffer.end(); ++iter0){
		if(*iter0 != 0){
			delete *iter0;
		}
	}

	for(std::list<download *>::iterator iter0 = DownloadBuffer.begin(); iter0 != DownloadBuffer.end(); ++iter0){
		delete *iter0;
	}
	}//end lock scope

	delete sendPending;
	delete downloadComplete;
}

void client::checkTimeouts()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);

	int CB_size = ClientBuffer.size();
	for(int socketfd=0; socketfd<CB_size; ++socketfd){
		if(ClientBuffer[socketfd] != 0){
			if(currentTime - ClientBuffer[socketfd]->get_lastSeen() >= global::TIMEOUT){
#ifdef DEBUG
				std::cout << "error: client::checkTimeouts() triggering disconnect of socket " << socketfd << "\n";
#endif
				disconnect(socketfd);

				//disconnect may resize ClientBuffer
				CB_size = ClientBuffer.size();
			}
		}
	}

	}//end lock scope
}

/*
This function should only be used from within a ClientDownloadBufferMutex.
This function may resize ClientBuffer, be careful calling it from within a loop!
*/
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

	std::list<download *>::iterator iter_cur, iter_end;
	iter_cur = DownloadBuffer.begin();
	iter_end = DownloadBuffer.end();
	while(iter_cur != iter_end){
		float bytesDownloaded = (*iter_cur)->getLatestRequest() * (global::BUFFER_SIZE - global::S_CTRL_SIZE);
		int fileSize = (*iter_cur)->getFileSize();
		float percent = (bytesDownloaded / fileSize) * 100;
		int percentComplete = int(percent);

		infoBuffer info;
		info.hash = (*iter_cur)->getHash();
		info.server_IP.splice(info.server_IP.end(), (*iter_cur)->get_IPs()); //get all IP's
		info.fileName = (*iter_cur)->getFileName();
		info.fileSize = fileSize;
		if(percentComplete > 100){
			info.percentComplete = 100;
		}
		else{
			info.percentComplete = percentComplete;
		}
		info.speed = (*iter_cur)->getSpeed();

		downloadInfo.push_back(info);
		++iter_cur;
	}
	}//end lock scope

	return true;
}

int client::getTotalSpeed()
{
	int speed = 0;

	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);
	std::list<download *>::iterator iter_cur, iter_end;
	iter_cur = DownloadBuffer.begin();
	iter_end = DownloadBuffer.end();
	while(iter_cur != iter_end){
		speed += (*iter_cur)->getSpeed();
		++iter_cur;
	}
	}//end lock scope

	return speed;
}

void client::main_thread()
{
	timeval tv;

	while(true){

		currentTime = time(0);
		if(currentTime - previousIntervalEnd > global::TIMEOUT){
			checkTimeouts();
			previousIntervalEnd = currentTime;
		}

		if(*downloadComplete){
			removeCompleted();
		}

		//trigger generation of new requests for ready sockets
		prepareRequests();

		/*
		These must be initialized every iteration on linux(and possibly other OS's)
		because linux will change them(POSIX.1-2001 allows this) to reflect the
		time that select() has blocked for.
		*/
		tv.tv_sec = 1;
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
	//if the socket is already connected, add the download but do not make a new connection
	int socketfd = 0;
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);
	int CB_size = ClientBuffer.size();
	for(int x=0; x<CB_size; ++x){
		if(ClientBuffer[x] != 0){
			if(PC->server_IP == ClientBuffer[x]->get_IP()){
				socketfd = x;
			}
		}
	}
	}//end lock scope

	if(socketfd == 0){
		//create new connection
		sockaddr_in dest_addr;
		socketfd = socket(PF_INET, SOCK_STREAM, 0);
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(global::P2P_PORT);
		dest_addr.sin_addr.s_addr = inet_addr(PC->server_IP.c_str());
		memset(&(dest_addr.sin_zero),'\0',8);

		if(connect(socketfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0){
			return false; //new connection failed
		}
		else{
#ifdef DEBUG
			std::cout << "info: client::newConnection() created socket " << socketfd << " for " << PC->server_IP << "\n";
#endif
			FD_SET(socketfd, &masterfds);
		}
	}

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
	std::vector<clientBuffer *>::iterator iter_cur, iter_end;
	iter_cur = ClientBuffer.begin();
	iter_end = ClientBuffer.end();
	while(iter_cur != iter_end){
		if(*iter_cur != 0){
			(*iter_cur)->prepareRequest();
		}
		++iter_cur;
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
	//locate completed downloads
	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);
	std::list<std::string> completeHash;
	std::list<download *>::iterator iter_cur, iter_end;
	iter_cur = DownloadBuffer.begin();
	iter_end = DownloadBuffer.end();
	while(iter_cur != iter_end){
		if((*iter_cur)->complete()){
			completeHash.push_back((*iter_cur)->getHash());
		}
		++iter_cur;
	}

	/*
	If termination can be done in all elements that contain the download then
	remove the Download element.
	*/
	std::list<std::string>::iterator hash_iter_cur, hash_iter_end;
	hash_iter_cur = completeHash.begin();
	hash_iter_end = completeHash.end();
	while(hash_iter_cur != hash_iter_end){
		bool terminated = true;
		/*
		Attempt to terminate the download with all ClientBuffer elements
		WARNING: disconnect() may change the size of ClientBuffer.
		*/
		for(int socketfd = 0; socketfd < ClientBuffer.size(); socketfd++){
			if(ClientBuffer[socketfd] != 0){
				if(ClientBuffer[socketfd]->terminateDownload(*hash_iter_cur)){
					if(ClientBuffer[socketfd]->empty()){
						disconnect(socketfd);
					}
				}
				else{
					terminated = false;
				}
			}
		}

		if(terminated){
			//termination successfull, remove the Download element
			std::list<download *>::iterator iter_cur, iter_end;
			iter_cur = DownloadBuffer.begin();
			iter_end = DownloadBuffer.end();
			while(iter_cur != iter_end){
				if(*hash_iter_cur == (*iter_cur)->getHash()){
					ClientIndex.terminateDownload((*iter_cur)->getHash());
					delete *iter_cur;
					DownloadBuffer.erase(iter_cur);
					break;
				}
				++iter_cur;
			}

			//remove the completeHash element, termination successful
			hash_iter_cur = completeHash.erase(hash_iter_cur);
		}
		else{ //termination failed
			++hash_iter_cur;
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
	int lastBlockSize = atoi(info.fileSize.c_str()) % \
		(global::BUFFER_SIZE - global::S_CTRL_SIZE) + global::S_CTRL_SIZE;
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

	//queue all sockets to be connected
	{//begin lock scope
	boost::mutex::scoped_lock lock(PendingConnectionMutex);
	for(int x=0; x<info.server_IP.size(); ++x){
		//add to existing container if possible
		bool found = false;
		std::list<std::list<pendingConnection *> >::iterator iter_cur, iter_end;
		iter_cur = PendingConnection.begin();
		iter_end = PendingConnection.end();
		while(iter_cur != iter_end){
			if(iter_cur->back()->server_IP == info.server_IP[x]){
				iter_cur->push_front(new pendingConnection(Download, info.server_IP[x], info.file_ID[x]));
				found = true;
			}
			++iter_cur;
		}

		if(!found){
			std::list<pendingConnection *> temp;
			temp.push_back(new pendingConnection(Download, info.server_IP[x], info.file_ID[x]));
			PendingConnection.push_back(temp);
		}
	}
	}//end lock scope

	for(int x=0; x<info.server_IP.size(); ++x){
		boost::thread T(boost::bind(&client::serverConnect_thread, this));
	}

	return true;
}

//this function is untested
bool client::startNewConnection(const std::string hash, std::string server_IP, std::string file_ID)
{
	download * Download = 0;

	{//begin lock scope
	boost::mutex::scoped_lock lock(ClientDownloadBufferMutex);
	std::list<download *>::iterator iter_cur, iter_end;
	iter_cur = DownloadBuffer.begin();
	iter_end = DownloadBuffer.end();
	while(iter_cur != iter_end){
		if((*iter_cur)->getHash() == hash){
			Download = *iter_cur;
		}
		++iter_cur;
	}
	}//end lock scope

	if(Download == 0){
		return false;
	}

	{//begin lock scope
	boost::mutex::scoped_lock lock(PendingConnectionMutex);
	//add to existing container if possible
	bool found = false;
	std::list<std::list<pendingConnection *> >::iterator iter_cur, iter_end;
	iter_cur = PendingConnection.begin();
	iter_end = PendingConnection.end();
	while(iter_cur != iter_end){
		if(iter_cur->back()->server_IP == server_IP){
			iter_cur->push_front(new pendingConnection(Download, server_IP, file_ID));
			found = true;
		}
		++iter_cur;
	}

	if(!found){
		std::list<pendingConnection *> temp;
		temp.push_back(new pendingConnection(Download, server_IP, file_ID));
		PendingConnection.push_back(temp);
	}
	}//end lock scope

	return true;
}

void client::serverConnect_thread()
{
	/*
	This function will be run with multiple threads because it calls newConnection()
	which contains a blocking system call ( connect() ). The main purpose of this
	function is to avoid sending newConnection() two PCs with the same IPs at the
	same time. If that happened it would cause multiple connections to the same
	host/port.
	*/

	while(true){

		pendingConnection * PC = 0; //holds PC to process

		/*
		If a PC is not already being processed by another thread then claim it and
		mark it as PROCESSING.
		*/
		{//begin lock scope
		boost::mutex::scoped_lock lock(PendingConnectionMutex);
		if(!PendingConnection.empty()){
			std::list<std::list<pendingConnection *> >::iterator iter_cur, iter_end;
			iter_cur = PendingConnection.begin();
			iter_end = PendingConnection.end();
			while(iter_cur != iter_end){
				if(iter_cur->back()->status == FREE){
					PC = iter_cur->back();
					iter_cur->back()->status = PROCESSING;
					break;
				}
				++iter_cur;
			}
		}
		}//end lock scope

		if(PC != 0){
			newConnection(PC);

			/*
			Erase the PC that has already been processed, remove the PC's container
			if it is empty after the erasure.
			*/
			{//begin lock scope
			boost::mutex::scoped_lock lock(PendingConnectionMutex);
			std::list<std::list<pendingConnection *> >::iterator iter_cur, iter_end;
			iter_cur = PendingConnection.begin();
			iter_end = PendingConnection.end();
			while(iter_cur != iter_end){
				if(iter_cur->back() == PC){
					delete iter_cur->back();
					iter_cur->pop_back();
					if(iter_cur->empty()){
						PendingConnection.erase(iter_cur);
					}
					break;
				}
				++iter_cur;
			}
			}//end lock scope

			PC = 0;
		}
		else{
			/*
			End the thread if there are no more pending connections to be made.
			*/
			{//begin lock scope
			boost::mutex::scoped_lock lock(PendingConnectionMutex);
			if(PendingConnection.empty()){
				break;
			}
			}//end lock scope

			/*
			If a thread is waiting on newConnection for one of two PCs (that both
			have the same IP address it will cause other threads to wait for that
			connection attempt to finish. This sleep is to stop the threads from
			using all of the CPU checking a million times a second.
			*/
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
	for(std::list<download *>::iterator iter0 = DownloadBuffer.begin(); iter0 != DownloadBuffer.end(); ++iter0){
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

