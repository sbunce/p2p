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
#include <ctime>
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
		int fileSize;        //bytes
		int speed;           //bytes per second
		int percentComplete; //0-100
	};

	client();
	~client();
	/*
	getDownloadInfo   - returns download info for all files in sendBuffer
	                  - returns false if no downloads
	getTotalSpeed     - returns the total download speed(in bytes per second)
	start             - start the threads needed for the client
	startDownload     - schedules a download to be started
	stopDownload      - marks a download as completed so it will be stopped
	*/
	bool getDownloadInfo(std::vector<infoBuffer> & downloadInfo);
	int getTotalSpeed();
	void start();
	bool startDownload(exploration::infoBuffer info);
	void stopDownload(const std::string & hash);

private:
	//holds the current time(set in main_thread), used for server timeouts
	time_t currentTime;
	time_t previousIntervalEnd;

	/*
	The status of the pendingConnection element.
	FREE       - awaiting a thread to feed it to newConnection
	PROCESSING - a thread has already fed the element to newConnection

	If a element is set to processing then no other threads touch it.
	*/
	enum PendingStatus { FREE, PROCESSING };
	class pendingConnection
	{
	public:
		pendingConnection(download * Download_in, std::string & server_IP_in,
			std::string & file_ID_in)
		{
			Download = Download_in;
			server_IP = server_IP_in;
			file_ID = file_ID_in;
			status = FREE;
		}

		download * Download;
		std::string server_IP;
		std::string file_ID;
		PendingStatus status;
	};

	/*
	Holds connections which need to be made. If multiple connections to the same
	server need to be made they're put in the same inner vector.
	*/
	std::list<std::list<pendingConnection *> > PendingConnection;

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
	The vector holds the hash of downloads scheduled for deferred termination.
	*/
	std::vector<std::string> deferredTermination;

	//true if a download is complete, handed to all downloads
	bool * downloadComplete;

	/*
	checkTimeouts        - checks all servers to see if they've timed out and removes/disconnects if they have
	disconnect           - disconnects a socket, modifies ClientBuffer
	main_thread          - main client thread that sends/receives data and triggers events
	newConnection        - create a connection with a server, modifies ClientBuffer
	                     - returns false if cannot connect to server
	prepareRequests      - touches each ClientBuffer element to trigger new requests(if needed)
	read_socket          - when a socket needs to be read
	removeCompleted      - removes downloads that are complete(or stopped)
	serverConnect_thread - sets up new downloads (monitors scheduledDownload)
	startNewConnection   - associates a new server with a download(connects to server)
	                     - returns false if the download associated with hash isn't found
	write_socket         - when a socket needs to be written
	*/
	void checkTimeouts();
	inline void disconnect(const int & socketfd);
	void main_thread();
	bool newConnection(pendingConnection * PC);
	void prepareRequests();
	inline void read_socket(const int & socketfd);
	void removeCompleted();
	void serverConnect_thread();
	bool startNewConnection(const std::string hash, std::string server_IP, std::string file_ID);
	inline void write_socket(const int & socketfd);

	//each mutex names the object it locks in the format boost::mutex <object>Mutex
	boost::mutex ClientDownloadBufferMutex; //for both ClientBuffer and DownloadBuffer
	boost::mutex deferredTerminationMutex;
	boost::mutex PendingConnectionMutex;
	boost::mutex PCPMutex;
};
#endif
