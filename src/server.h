#ifndef H_SERVER
#define H_SERVER

//boost
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

//std
#include <string>
#include <queue>

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

	//element of sendBuffer, public but outside classes shouldn't need one
	class bufferElement
	{
	public:
		std::string client_IP; //only used for info purposes like calculateSpeed()
		int clientSock;        //what socket to send the data to
		int file_ID;           //what file the client has requested
		int fileBlock;         //what chunk of the file the client wants(buffer*offset)
	};

	//element of uploadSpeed, public but outside classes shouldn't need one
	class speedElement
	{
	public:
		std::string client_IP;
		std::string fileName;
		int file_ID;           //file identifier, used to check against bufferElement
		int fileSize;          //size of file
		int fileBlock;         //what fileBlock was last requested
		int percentComplete;   //0-100

		int lastSeen;          //when the file was last seen in the uploadBuffer
		int lastCalc;          //time when uploadSpeed last calculated
		int oldFileBlock;      //fileBlock that was served at lastCalc
		int uploadSpeed;       //the speed in bytes/second of upload
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

	//networking related
	int maxConnections; //maximum connections allowed
	int numConnections; //how many are currently connected

	//having these class wide saves on tons of parameter passing
	fd_set readfds;   //file descriptor set(the one passed to select())
	fd_set masterfds; //master file descriptor set
	int fdmax;        //holds the number of the maximum socket

	//send buffer
	std::vector<bufferElement> sendBuffer;

	//used by getUploadInfo function to track upload speeds
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
	void calculateSpeed();
	void newCon(int listener);
	void queueRequest(int clientSock, char recvBuff[], int nbytes);
	int sendBlock(int clientSock, int fileID, int fileBlock);
	void stateTick();
	void start_thread();

	//items that require locking: uploadSpeed
	boost::mutex Mutex;

	//keeps track of shared files
	serverIndex ServerIndex;
};
#endif
