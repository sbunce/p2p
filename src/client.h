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
	getDownloadInfo    - returns download info for all files in sendBuffer
	                   - returns false if no downloads
	getTotalSpeed      - returns the total download speed
	start              - start the client thread
	startDownload      - start a new download
	terminateDownload  - schedule a download to be terminated
	*/

	bool getDownloadInfo(std::vector<infoBuffer> & downloadInfo);
	std::string getTotalSpeed();
	void start();
	bool startDownload(exploration::infoBuffer info);
	void terminateDownload(std::string messageDigest_in);

private:
	//networking related
	int fdmax;             //holds the number of the maximum socket
	fd_set readfds;        //temporary file descriptor set to pass to select()
	fd_set masterfds;      //master file descriptor set
	bool resumedDownloads; //true if postResumeConnect needs to be run(after resuming)

	//send buffer
	std::vector<download> downloadBuffer;

	/*
	Each deque will contain pointers to serverElements that one or more of the
	downloads have. Before each sendPendingRequests a token is passed from one
	serverElement to the next. This will determine what download gets to make a
	request. This is for the purpose of allowing multiple downloads from the same
	server to share one socket and take turns using it.
	*/
	std::vector<std::deque<download::serverElement *> > serverHolder;

	/*
	If the download and serverElements are removed when the server has not sent
	all of it's data there is a condition where a partial bucket is lost and the
	bucket for the wrong download is partially filled. A new request is not being
	sent back to that server because the download thinks it needs more data and
	won't relinquish it's token. To correct this terminateDownload() tells
	processBuffer() to ignore X number of bytes from a socket.
	*/
//DEBUG do something

	/*
	disconnect          - disconnects a socket
	disconnect          - disconnects a socket
	processBuffer       - establishes the receive protocol
	removeCompleted     - removes completed downloads from the sendBuffer
	start_thread        - starts the main client thread
	sendPendingRequests - send pending requests(duh)
	sendRequest         - sends a request to a server
	*/
	void disconnect(int socketfd);
	void postResumeConnect();
	void processBuffer(int socketfd, char recvBuff[], int nbytes);
	void removeCompleted(std::string messageDigest);
	void start_thread();
	void sendPendingRequests();
	int sendRequest(std::string server_IP, int & socketfd, int fileID, int fileBlock);

	boost::mutex downloadBufferMutex;  //mutex for any usage of sendBuffer
	boost::mutex masterfdsMutex;       //mutex for masterfds fd_set

	clientIndex ClientIndex;
};
#endif
