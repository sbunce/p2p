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
	download(const std::string & hash_in, const std::string & fileName_in, 
		const std::string & filePath_in, const int & fileSize_in, const int & blockCount_in,
		const int & lastBlock_in, const int & lastBlockSize_in, const int & lastSuperBlock_in,
		const int & currentSuperBlock_in);
	/*
	addBlock         - add complete fileBlocks to the superBlocks
	completed        - returns true if download completed, false if not
	getFileName      - returns the fileName of the download
	getFileSize      - returns the fileSize of the file being downloaded(bytes)
	getRequst        - returns the number of a needed block
	getSpeed         - returns the speed of this download
	*/
	void addBlock(const int & blockNumber, std::string & block);
	bool complete();
	const std::string & getFileName();
	const int & getFileSize();
	const int & getBytesExpected();
	const int getRequest();
	const int & getSpeed();

private:
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

	//these will be set automatically in ctor
	int downloadSpeed;     //speed of download(bytes per second)
	bool downloadComplete; //true when download is completed
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

