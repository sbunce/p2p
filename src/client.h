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
	getDownloadInfo   - returns download info for all files in sendBuffer
	                  - returns false if no downloads
	getTotalSpeed     - returns the total download speed
	start             - start the client thread
	startDownload     - start a new download
	terminateDownload - wrapper for terminateDownload()
	*/
	bool getDownloadInfo(std::vector<infoBuffer> & downloadInfo);
	std::string getTotalSpeed();
	void start();
	bool startDownload(exploration::infoBuffer info);
	void stopDownload(std::string messageDigest_in);

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
	removeAbusive_check       - returns true if the serverElement is marked as abusive
	removeAbusive_checkDelete - same as removeAbusive_check but disconnects socket and frees memory
	disconnect                - disconnects a socket
	processBuffer             - establishes the receive protocol
	removeAbusive             - disconnects sockets and removes abusive servers
	removeTerminated          - removes downloads that are done terminating
	start_thread              - starts the main client thread
	sendPendingRequests       - send pending requests(duh)
	sendRequest               - sends a request to a server
	terminateDownload_real    - terminate a download, all calls to this function need to be
	                          - within a downloadBufferMutex scoped lock
	*/
	bool removeAbusive_check(download::serverElement * ringElement);
	bool removeAbusive_checkDelete(download::serverElement * ringElement);
	void disconnect(int socketfd);
	void postResumeConnect();
	void processBuffer(int socketfd, char recvBuff[], int nbytes);
	void removeAbusive();
	void removeTerminated();
	void start_thread();
	void sendPendingRequests();
	int sendRequest(std::string server_IP, int & socketfd, int fileID, int fileBlock);

	/*
	This function is only to be called if the download isn't expecting any data on any
	sockets. If the download is expecting data and we delete the download then there
	are lots of synchronization problems which lead to "hung" sockets(program logic
	out of sync with what the socket can do). For this reason disconnected downloads
	are deferred with stopDownload()/removeTerminated() and only disconnected
	when all serverElements expect 0 bytes from their respective servers. Any calls
	to this function must be within a downloadBufferMutex scoped lock.
	*/
	void terminateDownload(std::string messageDigest_in);

	boost::mutex downloadBufferMutex; //mutex for any usage of sendBuffer
	boost::mutex masterfdsMutex;      //mutex for any usage of masterfds fd_set

	clientIndex ClientIndex;
};
#endif
