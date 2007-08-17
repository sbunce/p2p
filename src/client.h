#ifndef H_CLIENT
#define H_CLIENT

//boost
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

//networking
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

//contains enum type for error codes select() might return
#include <errno.h>

//std
#include <list>
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
		std::string hash;
		std::string server_IP;
		std::string fileName;
		std::string fileSize;
		std::string speed;
		int percentComplete;
	};

	client();
	/*
	getDownloadInfo   - returns download info for all files in sendBuffer
	                  - returns false if no downloads
	getTotalSpeed     - returns the total download speed(in bytes per second)
	start             - start the client thread
	startDownload     - start a new download
	stopDownload      - wrapper for terminateDownload()
	*/
	bool getDownloadInfo(std::vector<infoBuffer> & downloadInfo);
	int getTotalSpeed();
	void start();
	bool startDownload(exploration::infoBuffer info);
	void stopDownload(const std::string & hash);

private:
	sha SHA; //creates messageDigests

	//networking related
	int fdmax;            //holds the number of the maximum socket
	fd_set masterfds;     //master file descriptor set
	fd_set readfds;       //holds what sockets are ready to be read
	fd_set writefds;      //holds what sockets can be written to without blocking
	bool resumeDownloads; //true if postResumeConnect needs to be run(after resuming)

	/*
	Buffer for partial sends. The partial send buffers can be accessed by socket
	number with sendBuffer[socketNumber];
	*/
	std::vector<std::string> sendBuffer;

	//contains currently running downloads
	std::list<download> downloadBuffer;

	/*
	Each deque will contain pointers to serverElements that one or more of the
	downloads have. Before each sendPendingRequests a token is passed from one
	serverElement to the next. This will determine what download gets to make a
	request. This is for the purpose of allowing multiple downloads from the same
	server to share one socket and take turns using it.
	*/
	std::list<std::list<download::serverElement *> > serverHolder;

	/*
	disconnect                  - disconnects a socket
	newConnection               - create a connection with a server, returns false if failed
	processBuffer               - establishes the receive protocol
	resetHungDownloadSpeed      - checks for downloads without servers and resets their download speeds
	removeAbusive_checkContains - returns true if the vector contains a serverElement with a matching IP
	removeAbusive               - disconnects sockets and removes abusive servers
	removeDisconnected          - removes serverElements for servers that disconnected from us
	removeTerminated            - removes downloads that are done terminating
	start_thread                - starts the main client thread
	sendPendingRequests         - send pending requests
	encode                      - converts 32bit integer to 4 chars
	sendRequest                 - sends a request to a server
	*/
	void disconnect(const int & socketfd);
	bool newConnection(int & socketfd, std::string & server_IP);
	void postResumeConnect();
	void processBuffer(const int & socketfd, char recvBuff[], const int & nbytes);
	void resetHungDownloadSpeed();
	bool removeAbusive_checkContains(std::list<download::abusiveServerElement> & abusiveServerTemp, download::serverElement * SE);
	void removeAbusive();
	void removeDisconnected(const int & socketfd);
	void removeTerminated();
	void start_thread();
	void sendPendingRequests();
	std::string encodeInt(unsigned int number);
	int sendRequest(const int & socketfd, const unsigned int & fileID, const unsigned int & fileBlock);

	/*
	This function is only to be called if the download isn't expecting any data on any
	sockets. If the download is expecting data and we delete the download then there
	are lots of synchronization problems which lead to "hung" sockets(program logic
	out of sync with what the socket can do). For this reason disconnected downloads
	are deferred with stopDownload()/removeTerminated() and only disconnected
	when all serverElements expect 0 bytes from their respective servers. Any calls
	to this function must be within a downloadBufferMutex scoped lock.
	*/
	void terminateDownload(const std::string & hash);

	boost::mutex Mutex;

	clientIndex ClientIndex;
};
#endif
