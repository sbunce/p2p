#ifndef H_CLIENT_BUFFER
#define H_CLIENT_BUFFER

#include <deque>
#include <list>
#include <string>
#include <vector>

#include "superBlock.h"
#include "globals.h"

class download
{
public:
	class serverElement
	{
	public:
		std::string server_IP; //IP of the server that has the file
		int socketfd;          //holds the socket number or -1(no socket yet created)
		bool ready;            //true if ready to send another block request
		int file_ID;           //the ID of the file on the server
		std::string bucket;    //buffer for received data
		bool lastRequest;      //the last block has been requested from this server
	};

	//stores information specific to certain servers
	std::list<serverElement> Server;

	download(std::string & messageDigest_in, std::string & fileName_in, 
		std::string & filePath_in, int fileSize_in, int blockCount_in, int lastBlock_in,
		int lastBlockSize_in, int lastSuperBlock_in, int currentSuperBlock_in);
	~download();
	/*
	completed        - returns true if download completed, false if not
	getBlockCount    - returns the number of fileBlocks received
	getMessageDigest - returns the message digest for the download
	getFileName      - returns the fileName of the download
	getFileSize      - returns the fileSize of the file being downloaded(bytes)
	getLastBlock     - returns the last block number
	getRequst        - returns a number of a block that needs to be requested
	getSpeed         - returns the speed of this download
	hasSocket        - returns true if the socket belongs to this download
	processBuffer    - does actions based on buffer
	*/
	void addServer(download::serverElement & Server);
	bool complete();
	const int & getBlockCount();
	const std::string & getFileName();
	const int & getFileSize();
	const int & getLastBlock();
	const std::string & getMessageDigest();
	int getRequest();
	const int & getSpeed();
	bool hasSocket(int socketfd);
	void processBuffer(int socketfd, char recvBuff[], int nbytes);

private:
	//these must be set before the download begins and will be set by ctor
	std::string messageDigest; //unique identifier of the file and message digest
	std::string fileName;      //name of the file
	std::string filePath;      //path to write file to on local system
	int fileSize;              //size of the file(bytes)
	int blockCount;            //used for percentage complete calculation
	int lastBlock;             //the last block number
	int lastBlockSize;         //holds the exact size of the last fileBlock(in bytes)
	int lastSuperBlock;        //holds the number of the last superBlock
	int currentSuperBlock;     //what superBlock clientBuffer is currently on

	//these will be set automatically in ctor
	int downloadSpeed;     //speed of download(bytes per second)
	bool downloadComplete; //true when download is completed
	int averageSeconds;    //seconds to average download speed over(n - 2 seconds)

	/*
	This will grow to SUPERBUFFER_SIZE if there are missing blocks in a superBlock.
	This buffer is maintained to avoid the problem of rerequesting blocks that slow
	hosts havn't sent yet. Hopefully a slow host will finish sending it's block by
	the time SUPERBUFFER_SIZE superBlocks have completed. If it hasn't then it will
	get requested from a different host.
	*/
	std::deque<superBlock> superBuffer;

	//these vectors are parallel and used for download speed calculation
	std::vector<int> downloadSecond; //second at which secondBytes were downloaded
	std::vector<int> secondBytes;    //bytes in the second

	/*
	addBlocks        - add complete fileBlocks to the superBlocks
	calculateSpeed   - calculates the download speed of a download
	findMissing      - finds missing blocks in the tree
	writeTree        - writes a superBlock to file
	*/
	int addBlock(std::string & bucket);
	void calculateSpeed();
	void findMissing();
	void writeSuperBlock(std::string * container[]);
};
#endif

