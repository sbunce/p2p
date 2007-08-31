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

#include "clientBuffer.h"
#include "clientIndex.h"
#include "download.h"
#include "exploration.h"
#include "global.h"

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
	~client();
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

	/*
	All the information relevant to each socket accessible with the socket fd int
	example: ClientBuffer[socketfd].something.

	DownloadBuffer stores all the downloads contained within the ClientBuffer in
	no particular order.

	Access to both of these must be locked with the same mutex.
	*/
	std::vector<clientBuffer *> ClientBuffer;
	//std::list<download **> DownloadBuffer;

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
	int * sendPending;

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
	disconnect                  - disconnects a socket
	encodeInt                   - converts 32bit integer to 4 chars
	newConnection               - create a connection with a server, returns false if failed
	prepareRequests             - sets up new requests
	postResumeConnect           - if the program was restarted this function resumes downloads
	resetHungDownloadSpeed      - checks for downloads without servers and resets their download speeds
	removeAbusive               - disconnects sockets and removes abusive servers
	removeDisconnected          - removes serverElements for servers that disconnected from us
	removeTerminated            - removes downloads that are done terminating
	start_thread                - starts the main client thread
	read_socket                 - when a socket needs to be read
	write_socket                - when a socket needs to be written
	sendRequest                 - sends a request to a server
	*/
	void disconnect(const int & socketfd);
	bool newConnection(int & socketfd, std::string & server_IP);
	void prepareRequests();
	void postResumeConnect();
	void resetHungDownloadSpeed();
	void removeAbusive();
	void removeDisconnected(const int & socketfd);
	void removeTerminated();
	void start_thread();
	inline void read_socket(const int & socketfd);
	inline void write_socket(const int & socketfd);
	int sendRequest(const int & socketfd, const unsigned int & fileID, const unsigned int & fileBlock);
	void startDeferredDownloads();
	bool startDownload_deferred(exploration::infoBuffer info);
	void terminateDownload(const std::string & hash);

	//each mutex names the object it locks in the format boost::mutex <object>Mutex
	boost::mutex ClientBufferMutex;
	boost::mutex deferredTerminationMutex;
	boost::mutex scheduledDownloadMutex;
};
#endif
