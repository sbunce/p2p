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
	startDownload     - schedules a download to be started
	stopDownload      - wrapper for terminateDownload()
	*/
	bool getDownloadInfo(std::vector<infoBuffer> & downloadInfo);
	int getTotalSpeed();
	void start();
	void startDownload(exploration::infoBuffer info);
	void stopDownload(const std::string & hash);

private:
	sha SHA;                 //creates messageDigests
	clientIndex ClientIndex; //gives client access to the database

	//networking related
	fd_set masterfds;     //master file descriptor set
	fd_set readfds;       //holds what sockets are ready to be read
	fd_set writefds;      //holds what sockets can be written to without blocking
	int fdmax;            //holds the number of the maximum socket

	//true if postResumeConnect needs to be run(after resuming)
	bool resumeDownloads;

	/*
	When this is zero there are no pending requests to send. This exists for the
	purpose of having an alternate select() call that doesn't involve writefds
	because using writefds hogs CPU.
	*/
	int sendPending;

	/*
	Buffer for partial sends. The partial send buffers can be accessed by socket
	number with sendBuffer[socketNumber];
	*/
	std::vector<std::string> sendBuffer;

	//contains currently running downloads
	std::list<download> downloadBuffer;

	/*
	exploration::infoBuffer's (new download information) is put here to schedule
	a download to start.
	*/
	std::list<exploration::infoBuffer> scheduledDownload;

	/*
	This holds the hash of a download scheduled for deferred termination. This
	exists for no other purpose but to make locking the downloadBuffer and
	serverHolder easier.
	*/
	std::vector<std::string> deferredTermination;

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
	encodeInt                   - converts 32bit integer to 4 chars
	newConnection               - create a connection with a server, returns false if failed
	postResumeConnect           - if the program was restarted this function resumes downloads
	prepareRequests             - send pending requests
	processBuffer               - establishes the receive protocol
	resetHungDownloadSpeed      - checks for downloads without servers and resets their download speeds
	removeAbusive_checkContains - returns true if the vector contains a serverElement with a matching IP
	removeAbusive               - disconnects sockets and removes abusive servers
	removeDisconnected          - removes serverElements for servers that disconnected from us
	removeTerminated            - removes downloads that are done terminating
	start_thread                - starts the main client thread
	read_socket                 - when a socket needs to be read
	write_socket                - when a socket needs to be written
	sendRequest                 - sends a request to a server
	*/
	void disconnect(const int & socketfd);
	std::string encodeInt(unsigned int number);
	bool newConnection(int & socketfd, std::string & server_IP);
	void postResumeConnect();
	void prepareRequests();
	void processBuffer(const int & socketfd, char recvBuff[], const int & nbytes);
	void resetHungDownloadSpeed();
	bool removeAbusive_checkContains(std::list<download::abusiveServerElement> & abusiveServerTemp, download::serverElement * SE);
	void removeAbusive();
	void removeDisconnected(const int & socketfd);
	void removeTerminated();
	void start_thread();
	void read_socket(const int & socket);
	void write_socket(const int & socket);
	int sendRequest(const int & socketfd, const unsigned int & fileID, const unsigned int & fileBlock);
	void startDeferredDownloads();
	bool startDownload_deferred(exploration::infoBuffer info);
	void terminateDownload(const std::string & hash);

	/*
	It's a lot of mutex's but each names the object it locks in the format
	boost::mutex <object>Mutex. The only possible complication to these locks is
	that a deferredTerminationMutex is nested within a downloadBufferMutex and a
	serverHolderMutex is nested within a downloadBufferMutex. As a consequence of
	this a downloadBufferMutex should never be locked within a deferredTerminationMutex
	and a serverHolderMutex should never be locked within a downloadBufferMutex.
	*/
	boost::mutex downloadBufferMutex;
	boost::mutex deferredTerminationMutex;
	boost::mutex sendBufferMutex;
	boost::mutex serverElementMutex;
	boost::mutex scheduledDownloadMutex;
};
#endif
