#ifndef H_DOWNLOAD
#define H_DOWNLOAD

#include <deque>
#include <list>
#include <string>
#include <vector>

#include "superBlock.h"
#include "global.h"

class download
{
public:

	//more will likely go in here later
	class connection
	{
	public:
		connection(std::string & server_IP_in)
		{
			server_IP = server_IP_in;
		}

		std::string server_IP;
	};

	download(const std::string & hash_in, const std::string & fileName_in, 
		const std::string & filePath_in, const int & fileSize_in, const int & blockCount_in,
		const int & lastBlock_in, const int & lastBlockSize_in, const int & lastSuperBlock_in,
		const int & currentSuperBlock_in, bool * downloadComplete_flag_in);

	/*
	addBlock         - add complete fileBlocks to the superBlocks
	completed        - returns true if download completed
	getBytesExpected - returns the number of bytes to expect for the latest request
	getFileName      - returns the fileName of the download
	getFileSize      - returns the fileSize of the file being downloaded(bytes)
	getHash          - returns the hash (UID) of this download
	get_IPs          - returns a list of all IPs this download associated with
	getLatestRequest - returns latestRequest
	getRequst        - returns the number of a needed block
	getSpeed         - returns the speed of this download
	regConnection    - registers a connection with the download
	unregConnection  - unregisters a connection with the download
	stop             - stops the download by marking it as completed
	*/
	void addBlock(const int & blockNumber, std::string & block);
	bool complete();
	const int & getBytesExpected();
	const std::string & getFileName();
	const int & getFileSize();
	const std::string & getHash();
	std::list<std::string> & get_IPs();
	const int & getLatestRequest();
	const int getRequest();
	const int & getSpeed();
	void regConnection(std::string & server_IP);
	void unregConnection(const std::string & server_IP);
	void stop();

private:
	//used by get_IPs
	std::list<std::string> get_IPs_list;

	/*
	clientBuffers register/unregister with the download by storing information
	here. This is useful to make it easy to get at network information without
	having to store any network logic in the download.
	*/
	std::list<connection> currentConnections;

	sha SHA; //creates message digests for superBlocks

	//these must be set before the download begins and will be set by ctor
	std::string hash;      //unique identifier of the file and message digest
	std::string fileName;  //name of the file
	std::string filePath;  //path to write file to on local system
	int fileSize;          //size of the file(bytes)
	int latestRequest;     //most recently requested block
	int lastBlock;         //the last block number
	int lastBlockSize;     //holds the exact size of the last fileBlock(in bytes)
	int lastSuperBlock;    //holds the number of the last superBlock
	int currentSuperBlock; //what superBlock clientBuffer is currently on

	/*
	Tells the client that a download is complete, used when download is complete.
	Full description in client.h.
	*/
	bool * downloadComplete_flag;

	//these will be set automatically in ctor
	int downloadSpeed;     //speed of download(bytes per second)
	bool downloadComplete; //true when the download is completed
	int averageSeconds;    //seconds to average download speed over(n - 2 seconds)

	/*
	This will grow to SUPERBUFFER_SIZE if there are missing blocks in a superBlock.
	This buffer is maintained to avoid the problem of rerequesting blocks that slow
	hosts havn't yet sent. Hopefully a slow host will finish sending it's block by
	the time SUPERBUFFER_SIZE superBlocks have completed. If it hasn't then it will
	get requested from a different host.
	*/
	std::deque<superBlock> superBuffer;

	//these vectors are parallel and used for download speed calculation
	std::vector<int> downloadSecond; //second at which secondBytes were downloaded
	std::vector<int> secondBytes;    //bytes in the second

	/*
	calculateSpeed  - calculates the download speed of a download
	findMissing - finds missing blocks in the tree
	writeTree   - writes a superBlock to file
	*/
	void calculateSpeed();
	void findMissing();
	void writeSuperBlock(std::string container[]);
};
#endif

