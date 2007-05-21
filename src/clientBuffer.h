#ifndef H_CLIENT_BUFFER
#define H_CLIENT_BUFFER

#include <string>
#include <vector>

#include "bst.h"
#include "globals.h"

class clientBuffer
{
public:
	//This is the SHA message digest. It is also the unique identifier for the file.
	std::string messageDigest;

	//if this is true all other blocks but the last block have been received
	bool requestLast;

//DEBUG, these are the 4 items that need to be generalized for multi-source
//perhaps a vector of structs containing these
	std::string server_IP; //IP of the server that has the file
	int socketfd;          //holds the socket number or -1(no socket yet created)
	bool ready;            //true if ready to send another block request
	int file_ID;           //the ID of the file on the server
	std::string bucket;    //buffer for received data

	std::string fileName;  //name of the file
	std::string filePath;  //path to write file to on local system
	int fileSize;          //size of the file(bytes)
	int blockCount;        //stores the number of blocks received, is not a offset from beginning!
	int lastBlock;         //the last block to download
	int lastBlockSize;     //holds the exact size of the last fileBlock(in bytes)
	int downloadSpeed;     //speed of download(bytes per second)

	clientBuffer();
	/*
	addToTree           - add a fileBlock to the binary search tree
	calculateSpeed      - calculates the download speed of a download
	completed           - returns true if download completed, false if not
	getBlockRuestNumber - returns a number of a block that needs to be requested
	hasSocket           - returns true if the socket belongs to this download
	writeTree           - writes as many blocks to the file as possible
	*/
	int addToTree(std::string & bucket);
	void calculateSpeed();
	bool completed();
	int getBlockRequestNumber();
	bool hasSocket(int socketfd_in);
	void writeTree();

private:
	int averageSeconds;       //seconds to average download speed over(n - 2 seconds)
	int lowestRequest;        //highest number in std::vector<int> request
	int highestRequest;       //lowest number in std::vector<int> request
	bst tree;                 //tree to reorder incoming out of order blocks
	int BF_offset;            //(BF_offset * BUFFER_FACTOR) determines start of new std::vector<int> request
	std::vector<int> request; //contains out-of-order block numbers to request(request buffer)

	//these vectors are parallel and used for download speed calculation
	std::vector<int> downloadSecond; //second at which secondBytes were downloaded
	std::vector<int> secondBytes;    //bytes in the second

	/*
	findMissing      - finds missing blocks in the tree
	generateRequests - refills the request buffer
	*/
	void findMissing();
	int generateRequests();
};
#endif

