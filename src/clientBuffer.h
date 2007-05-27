#ifndef H_CLIENT_BUFFER
#define H_CLIENT_BUFFER

#include <deque>
#include <list>
#include <string>
#include <vector>

#include "superBlock.h"
#include "globals.h"

class clientBuffer
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

	/*
	Each of the following items needs to be filled when the clientBuffer is instantiated.
	Server list needs to have at least one element in it.
	*/
	std::list<serverElement> Server; //holds information specific to individual servers
	std::string messageDigest; //unique identifier of the file and message digest
	std::string fileName;      //name of the file
	std::string filePath;      //path to write file to on local system
	int fileSize;              //size of the file(bytes)
	int blockCount;            //used for percentage complete calculation
	int lastBlock;             //the last block number
	int lastBlockSize;         //holds the exact size of the last fileBlock(in bytes)
	int lastSuperBlock;        //holds the number of the last superBlock


	std::vector<int> invalidSockets; //sockets clientBuffer wants disconnected(abusive/slow/etc)
	int downloadSpeed;               //speed of download(bytes per second)

	clientBuffer();
	/*
	addBlocks      - add complete fileBlocks to the superBlocks
	calculateSpeed - calculates the download speed of a download
	completed      - returns true if download completed, false if not
	getRuest       - returns a number of a block that needs to be requested
	hasSocket      - returns true if the socket belongs to this download
	processBuffer  - does actions based on buffer
	*/
	int addBlock(std::string & bucket);
	void calculateSpeed();
	bool complete();
	int getRequest();
	bool hasSocket(int socketfd);
	void processBuffer(int socketfd, char recvBuff[], int nbytes);

private:
	//true when download is completed and ready for client to remove
	bool downloadComplete;

	int averageSeconds;    //seconds to average download speed over(n - 2 seconds)
	int currentSuperBlock; //what superBlock clientBuffer is currently on

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
	findMissing      - finds missing blocks in the tree
	writeTree        - writes a completed superBlock to file
	*/
	void findMissing();
	void writeSuperBlock(std::string * container[]);
};
#endif

