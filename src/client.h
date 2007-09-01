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
		std::list<std::string> server_IP;
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
	stopDownload      - marks a download as completed so it will be stopped
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

	Access to both of these must be locked with the same mutex
	(ClientDownloadBufferMutex).
	*/
	std::vector<clientBuffer *> ClientBuffer;
	std::list<download *> DownloadBuffer;

	//networking related
	fd_set masterfds;     //master file descriptor set
	fd_set readfds;       //holds what sockets are ready to be read
	fd_set writefds;      //holds what sockets can be written to without blocking
	int fdmax;            //holds the number of the maximum socket

	/*
	When this is zero there are no pending requests to send. This exists for the
	purpose of having an alternate select() call that doesn't involve writefds
	because using writefds hogs CPU. When the value referenced by this equals 0
	then there are no sends pending and writefds doesn't need to be used. These
	are given to the clientBuffer elements so they can increment it.
	*/
	int * sendPending;

	/*
	exploration::infoBuffer's (new download information) is put here to schedule
	a download to start.
	*/
	bool newDownloadPending;
	std::list<exploration::infoBuffer> scheduledDownload;

	/*
	The vector holds the hash of a download scheduled for deferred termination.
	*/
	std::vector<std::string> deferredTermination;

	//true if a download is complete, handed to all downloads
	bool * downloadComplete;

	/*
	disconnect       - disconnects a socket
	newConnection    - create a connection with a server, returns false if failed
	prepareRequests  - sets up new requests
	removeCompleted  - removes downloads that are complete(or stopped)
	start_thread     - starts the main client thread
	read_socket      - when a socket needs to be read
	write_socket     - when a socket needs to be written
	*/
	void disconnect(const int & socketfd);
	bool newConnection(int & socketfd, std::string & server_IP);
	void prepareRequests();
	void removeCompleted();
	void start_thread();
	inline void read_socket(const int & socketfd);
	inline void write_socket(const int & socketfd);
	void startDeferredDownloads();
	bool startDownload_deferred(exploration::infoBuffer info);

	//each mutex names the object it locks in the format boost::mutex <object>Mutex
	boost::mutex ClientDownloadBufferMutex; //for both ClientBuffer and DownloadBuffer
	boost::mutex deferredTerminationMutex;
	boost::mutex scheduledDownloadMutex;
};
#endif
