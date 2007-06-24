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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
	getTotalSpeed - returns the total speed of all uploads
	*/
	bool getUploadInfo(std::vector<infoBuffer> & uploadInfo);
	std::string getTotalSpeed();
	const bool & isIndexing();
	void start();

private:
	//false while files are being indexed/hashed
	bool indexing;

	//element of sendBuffer, information about what was requested is stored in this
	class sendBufferElement
	{
	public:
		std::string client_IP; //only used for info purposes like calculateSpeed()
		int clientSock;        //what socket to send the data to
		int file_ID;           //what file the client has requested
		int fileBlock;         //what chunk of the file the client wants(buffer*offset)
	};

	/*
	If the client can't send REQUEST_CONTROL_SIZE bytes in one shot then the
	amount it can send will be stored here until all REQUEST_CONTROL_SIZE bytes
	are gotten. Once the full request is made it will be processed.
	*/
	class requestBufferElement
	{
	public:
		int lastSeen;
		int clientSock;
		std::string bucket; //holds the partial request
	};

	//use by uploadSpeed to track the progress of an upload
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

	//networking related
	int maxConnections; //maximum connections allowed
	int numConnections; //how many are currently connected
	fd_set readfds;   //file descriptor set(the one passed to select())
	fd_set masterfds; //master file descriptor set
	int fdmax;        //holds the number of the maximum socket

	//stores pending responses
	std::vector<sendBufferElement> sendBuffer;

	//stores partial requests
	std::vector<requestBufferElement> requestBuffer;

	//used by calculateSpeed() to track upload speeds
	std::vector<speedElement> uploadSpeed;

	/*
	disconnect     - disconnect client and remove socket from master set
	calculateSpeed - process the sendQueue and update uploadSpeed
	newCon         - sets up socket for client
	serveRequest   - the network protocol of this program, does actions based on request
	sendFile       - send a file to a client
	stateTick      - do actions pending, like sending data, etc
	start_th       - starts the server thread
	*/
	void disconnect(int clientSock);
	void calculateSpeed(int clientSock, int file_ID, int fileBlock);
	void newCon(int listener);
	void queueRequest(int clientSock, char recvBuff[], int nbytes);
	int sendBlock(int clientSock, int file_ID, int fileBlock);
	void stateTick();
	void start_thread();

	//items that require locking: uploadSpeed
	boost::mutex Mutex;

	//keeps track of shared files
	serverIndex ServerIndex;
};
#endif
