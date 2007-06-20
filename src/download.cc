//std
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef UNRELIABLE_CLIENT
#include <time.h>
#endif

#include "download.h"

download::download(std::string & messageDigest_in, std::string & fileName_in,
	std::string & filePath_in, int fileSize_in, int blockCount_in, int lastBlock_in,
	int lastBlockSize_in, int lastSuperBlock_in, int currentSuperBlock_in)
{
	//non-defaults
	messageDigest = messageDigest_in;
	fileName = fileName_in;
	filePath = filePath_in;
	fileSize = fileSize_in;
	blockCount = blockCount_in;
	lastBlock = lastBlock_in;
	lastBlockSize = lastBlockSize_in;
	lastSuperBlock = lastSuperBlock_in;
	currentSuperBlock = currentSuperBlock_in;

	//defaults
	downloadComplete = false;
	downloadSpeed = 0;

	//add the first superBlock to superBuffer, needed for getRequest to work on first call
	superBlock SB(currentSuperBlock++, lastBlock);
	superBuffer.push_back(SB);

#ifdef UNRELIABLE_CLIENT
	//seed random number generator
	std::srand(time(0));
#endif
}

download::~download()
{

}

int download::addBlock(std::string & bucket)
{
	calculateSpeed();

	//find position of delimiters
	int first = bucket.find_first_of(",");
	int second = bucket.find_first_of(",", first+1);
	int third = bucket.find_first_of(",", second+1);

	//get control data
	std::string command = bucket.substr(0, first);
	std::string file_ID = bucket.substr(first+1, (second - 1) - first);
	std::string blockNumber = bucket.substr(second+1, (third - 1) - second);

#ifdef UNRELIABLE_CLIENT
	//this exists for debug purposes, 1% chance of "dropping" a packet
	int random = rand() % 100;
	if(random < global::UNRELIABLE_CLIENT_PERCENT){
		std::cout << "testing: client::addToTree(): OOPs! I dropped fileBlock " << blockNumber << "\n";

		if(atoi(blockNumber.c_str()) == lastBlock){
			bucket.clear();
		}
		else{
			bucket.erase(0, global::BUFFER_SIZE);
		}

		return 0;
	}
#endif

	//prepare fileBlock to be added to a superBlock
	std::string fileBlock;
	if(bucket.size() >= global::BUFFER_SIZE){
		fileBlock = bucket.substr(global::CONTROL_SIZE, global::BUFFER_SIZE - global::CONTROL_SIZE);
		bucket.erase(0, global::BUFFER_SIZE);
	}
	else{ //last block encountered
		fileBlock = bucket.substr(global::CONTROL_SIZE, bucket.size() - global::CONTROL_SIZE);
		bucket.clear();
	}

	for(std::deque<superBlock>::iterator iter = superBuffer.begin(); iter != superBuffer.end(); iter++){
		if(iter->addBlock(atoi(blockNumber.c_str()), fileBlock)){
			break;
		}
	}
}

void download::calculateSpeed()
{
	time_t currentTime = time(0);

	bool updated = false;
	//if the vectors are empty
	if(downloadSecond.empty()){
		downloadSecond.push_back(currentTime);
		secondBytes.push_back(global::BUFFER_SIZE);
		updated = true;
	}

	//get rid of information that's older than what to average
	if(currentTime - downloadSecond.front() >= global::SPEED_AVERAGE + 2){
		downloadSecond.erase(downloadSecond.begin());
		secondBytes.erase(secondBytes.begin());
	}

	//if still on the same second
	if(!updated && downloadSecond.back() == currentTime){
		secondBytes.back() += global::BUFFER_SIZE;
		updated = true;
	}

	//no entry for current second, add one
	if(!updated){
		downloadSecond.push_back(currentTime);
		secondBytes.push_back(global::BUFFER_SIZE);
	}

	//count up all the bytes excluding the first and last second
	int totalBytes = 0;
	for(int x=1; x<downloadSecond.size() - 1; x++){
		totalBytes += secondBytes.at(x);
	}

	//divide by the time
	downloadSpeed = totalBytes / global::SPEED_AVERAGE;
}

bool download::complete()
{
	return downloadComplete;
}

const int & download::getBlockCount()
{
	return blockCount;
}

const std::string & download::getFileName()
{
	return fileName;
}

const int & download::getFileSize()
{
	return fileSize;
}

const int & download::getLastBlock()
{
	return lastBlock;
}

const std::string & download::getMessageDigest()
{
	return messageDigest;
}

int download::getRequest()
{
	/*
	This function is a bit hard to understand but it defines how the superBlocks
	are managed and is inherently complicated.
	*/

	/*
	Write out the oldest superBlocks if they're complete. It's possible the
	superBuffer could be empty so a check for this is done.
	*/
	if(!superBuffer.empty()){
		while(superBuffer.back().complete()){

			writeSuperBlock(superBuffer.back().container);
			superBuffer.pop_back();

			if(superBuffer.empty()){
				break;
			}
		}
	}

	/*
	Create another superBlock if the buffer is empty and the download is not
	completed.
	*/
	if(superBuffer.empty() && currentSuperBlock <= lastSuperBlock){
		superBlock SB(currentSuperBlock++, lastBlock);
		superBuffer.push_front(SB);
	}

	/*
	If all requests were made but the superBlock is not complete start working
	on a new superBlocks to give slower servers time to send their fileBlocks for
	the older superBlocks.

	.front() indicates a newer superBlock
	.back() indicates a older superBlock
	*/
	/*
	If all fileBlocks have been requested but the front is not complete either
	there are two possibilities:

	1) Add a new superBlock and get a request number from it.
	2) There is no room for another superBlock, rerequest a fileBlock from the
	   oldest superBlock.
	*/
	if(superBuffer.front().allRequested() && !superBuffer.front().complete()){

		//if the superBuffer is full rerequest from the oldest superBlock
		if(superBuffer.size() >= global::SUPERBUFFER_SIZE){
			blockCount = superBuffer.back().getRequest();
		}
		else{ //there is room for another superBlock

			//if the file needs another superBlock add it and get a request
			if(currentSuperBlock <= lastSuperBlock){
				superBlock SB(currentSuperBlock++, lastBlock);
				superBuffer.push_front(SB);
				blockCount = superBuffer.front().getRequest();
			}
			else{
				/*
				The file doesn't need another superBlock, start rerequesting early.
				This happens when the end of the download approaches.
				*/
				blockCount = superBuffer.back().getRequest();
			}
		}
	}
	/*
	If all requests have not yet been made then proceed normally by getting the
	next request from the newest superBlock.
	*/
	else if(!superBuffer.front().allRequested()){
		blockCount = superBuffer.front().getRequest();
	}
	/*
	All blocks already requested from the front superBlock and the front
	superBlock is complete. Rerequest a block from the oldest superBlock.
	*/
	else{
		blockCount = superBuffer.back().getRequest();
	}

	/*
	Don't return the last block number until it's the absolute last needed by
	this download. This makes checking for a completed download a lot easier!
	*/
	if(blockCount == lastBlock){

		//determine if there are no superBlocks left but the last one
		if(superBuffer.size() == 1){

			/*
			If no superBlock but the last one remains then try getting another
			request. If the superBlock returns -1 then it doesn't need any other
			blocks.
			*/
			int request = superBuffer.back().getRequest();

			//if no other block but the last one is needed
			if(request == -1){
				return lastBlock;
			}
			else{ //if another block is needed
				return request;
			}
		}
		else{ //the superBuffer has more than one block in it, serve the oldest
			blockCount = superBuffer.back().getRequest();
			return blockCount;
		}
	}
	else{ //any request but the last one gets returned without the above checking
		return blockCount;
	}
}

const int & download::getSpeed()
{
	return downloadSpeed;
}

bool download::hasSocket(int socketfd, int nbytes)
{
	for(std::vector<serverElement *>::iterator iter0 = Server.begin(); iter0 != Server.end(); iter0++){

		/*
		This function is used by the client to check whether this download requested
		the fileBlock it got a response to. Only return true if the download both 
		has the socket and it has a token. If it doesn't have the token then
		the request came from a different download.
		*/
		if((*iter0)->socketfd == socketfd && (*iter0)->token && (*iter0)->bytesExpected >= nbytes){
			return true;
		}
	}

	return false;
}

void download::processBuffer(int socketfd, char recvBuff[], int nbytes)
{
	//fill the bucket
	for(std::vector<serverElement *>::iterator iter0 = Server.begin(); iter0 != Server.end(); iter0++){

		//add the buffer to the right bucket and process
		if((*iter0)->socketfd == socketfd){

			(*iter0)->bucket.append(recvBuff, nbytes);

#ifdef DEBUG_VERBOSE
			std::cout << "info: download::processBuffer() IP: " << (*iter0)->server_IP << " bucket size: " << (*iter0)->bucket.size() << "\n";
			std::cout << "info: superBuffer.size(): " << superBuffer.size() << "\n";
#endif

			//disconnect the server if it's being nasty!
			if((*iter0)->bucket.size() > 3*global::BUFFER_SIZE){
#ifdef DEBUG
				std::cout << "error: client::processBuffer() detected buffer overrun from " << (*iter0)->server_IP << "\n";
#endif
//DEBUG, add in something to disconnect sockets

				//invalidSockets.push_back(iter->socketfd);
				Server.erase(iter0);
			}

			//if full block received add it to a superBlock and ready another request
			if((*iter0)->bucket.size() % global::BUFFER_SIZE == 0){
				addBlock((*iter0)->bucket);
				(*iter0)->bytesExpected = 0;
				(*iter0)->token = false;
			}

			if((*iter0)->lastRequest){

				//check for zero in case the last requested block was exactly global::BUFFER_SIZE
				if((*iter0)->bucket.size() != 0){

					//add last partial block to tree
					if((*iter0)->bucket.size() == lastBlockSize){
						addBlock((*iter0)->bucket);

#ifdef UNRELIABLE_CLIENT
						/*
						This exists to make UNRELIABLE_CLIENT work. It wouldn't be needed
						under real conditions because if the program got to this point it
						wouldn't "drop" a fileBlock.
						*/
						if(!superBuffer.back().complete()){
							(*iter0)->bytesExpected = 0;
							(*iter0)->token = false;
						}
#endif
					}
					else{ //full block not yet received, wait for it
						break;
					}
				}

				//write out the oldest superBlock if it's complete
				while(superBuffer.back().complete()){

					writeSuperBlock(superBuffer.back().container);
					superBuffer.pop_back();

					if(superBuffer.empty()){
						downloadComplete = true;
						break;
					}
				}
			}
		}
	}
}

void download::writeSuperBlock(std::string container[])
{
#ifdef DEBUG_VERBOSE
	std::cout << "info: download::writeSuperBlock() was called\n";
#endif

	std::ofstream fout(filePath.c_str(), std::ios::app);

	if(fout.is_open()){

		for(int x=0; x<global::SUPERBLOCK_SIZE; x++){
			fout.write(container[x].c_str(), container[x].size());
		}
		fout.close();
	}
	else{
#ifdef DEBUG
		std::cout << "error: download::writeTree() error opening file\n";
#endif
	}
}

