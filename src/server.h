#ifndef H_SERVER
#define H_SERVER

//boost
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

//std
#include <deque>
#include <string>
#include <vector>

//networking
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "globals.h"
#include "serverIndex.h"

class server
{
public:
	//used to pass information to user interface
	class infoBuffer
	{
	public:
		std::string client_IP;
		std::string file_ID;
		std::string fileName;
		std::string fileSize;
		std::string speed;
		int percentComplete;
	};

	server();
	/*
	getUploadInfo - updates and returns upload information
	              - returns false if no uploads
	getTotalSpeed - returns the total speed of all uploads(in bytes per second)
	*/
	bool getUploadInfo(std::vector<infoBuffer> & uploadInfo);
	int getTotalSpeed();
	const bool & isIndexing();
	void start();

private:
	//true while files are being indexed/hashed
	bool indexing;

	//used by uploadSpeed to track the progress of an upload
	class speedElement
	{
	public:
		//the gui will need this when displaying the upload
		std::string fileName;

		//used to idenfity what upload this is
		std::string client_IP;
		int file_ID;

		//these vectors are parallel and used for upload speed calculation
		std::deque<int> downloadSecond; //second at which secondBytes were uploaded
		std::deque<int> secondBytes;    //bytes in the second

		//used to calculate percent complete
		int fileSize;  //size of file
		int fileBlock; //what fileBlock was last requested
	};

	//used by calculateSpeed() to track upload speeds
	std::vector<speedElement> uploadSpeed;

	/*
	Stores pending responses. The partial send buffers can be accessed by socket
	number with sendBuffer[socketNumber];
	*/
	std::vector<std::string> sendBuffer;

	/*
	Stores partial requests. The partial requests can be accessed by socket number
	with receiveBuffer[socketNumber];
	*/
	std::vector<std::string> receiveBuffer;

	//how many are connections currently established
	int connections;

	//select() related
	fd_set masterfds;   //master file descriptor set
	fd_set readfds;     //set when socket can read without blocking
	fd_set writefds;    //set when socket can write without blocking
	int fdmax;          //holds the number of the maximum socket

	/*
	decodeInt         - turns four char's in to a 32bit integer starting at 'begin'
	disconnect        - disconnect client and remove socket from master set
	calculateSpeed    - process the sendQueue and update uploadSpeed
	newConnection     - sets up socket for client
	prepareSendBuffer - prepares a response to a file block request
	processRequest    - interprets the request from a client and buffers it
	sendBlock         - sends a fileBlock to a client
	stateTick         - do pending actions
	*/
	unsigned int decodeInt(const int & begin, char recvBuffer[]);
	void disconnect(const int & socketfd);
	void calculateSpeed(const int & socketfd, const int & file_ID, const int & fileBlock);
	void newConnection(const int & listener);
	int prepareSendBuffer(const int & socketfd, const int & file_ID, const int & blockNumber);
	void processRequest(const int & socketfd, char recvBuffer[], const int & nbytes);
	void start_thread();

	//locks
	boost::mutex sendBufferMutex;
	boost::mutex receiveBufferMutex;
	boost::mutex uploadSpeedMutex;

	//keeps track of shared files
	serverIndex ServerIndex;
};
#endif
