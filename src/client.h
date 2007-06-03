#ifndef H_CLIENT
#define H_CLIENT

//boost
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

//networking
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

//std
#include <queue>
#include <string>
#include <vector>

#include "clientIndex.h"
#include "download.h"
#include "exploration.h"
#include "globals.h"

class client
{
public:
	//used to pass information to user interface
	class infoBuffer
	{
	public:
		std::string messageDigest;
		std::string server_IP;
		std::string fileName;
		std::string fileSize;
		std::string speed;
		int percentComplete;
	};

	client();
	/*
	terminateDownload  - removes a download from sendBuffer and disconnects all sockets associated with it
	getDownloadInfo    - returns download info for all files in sendBuffer
	                   - returns false if no downloads
	getTotalSpeed      - returns the total download speed
	start              - start the client thread
	startDownload      - start a new download
	*/
	void terminateDownload(std::string messageDigest_in);
	bool getDownloadInfo(std::vector<infoBuffer> & downloadInfo);
	std::string getTotalSpeed();
	void start();
	bool startDownload(exploration::infoBuffer info);

private:
	//networking related
	int fdmax;              //holds the number of the maximum socket
	fd_set readfds;         //master file descriptor set

	//send buffer
	std::vector<download> sendBuffer;

	/*
	disconnect          - disconnects a socket
	disconnect          - disconnects a socket
	processBuffer       - establishes our receive protocol,
	                    - does actions based on control data and server_IP
	removeCompleted     - removes completed downloads from the sendBuffer
	start_thread        - starts the main client thread
	sendPendingRequests - send pending requests(duh)
	sendRequest         - sends a request to a server
	*/
	void disconnect(int socketfd);
	int processBuffer(int socketfd, char recvBuff[], int nbytes);
	void removeCompleted(std::string messageDigest);
	void start_thread();
	void sendPendingRequests();
	int sendRequest(std::string server_IP, int & socketfd, int fileID, int fileBlock);

	boost::mutex sendBufferMutex;  //mutex for any usage of sendBuffer
	boost::mutex readfdsMutex;     //mutex for readfds fd_set

	clientIndex ClientIndex;
};
#endif
