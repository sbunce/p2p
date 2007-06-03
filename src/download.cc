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
	averageSeconds = 3;
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
		std::cout << "testing: client::addToTree(): OOPs! I dropped fileBlock " << blockNumber << std::endl;

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
	std::string * fileBlock = new std::string;
	if(bucket.size() >= global::BUFFER_SIZE){
		*fileBlock = bucket.substr(global::CONTROL_SIZE, global::BUFFER_SIZE - global::CONTROL_SIZE);
		bucket.erase(0, global::BUFFER_SIZE);
	}
	else{ //last block encountered
		*fileBlock = bucket.substr(global::CONTROL_SIZE, bucket.size() - global::CONTROL_SIZE);
		bucket.clear();
	}

	bool blockAdded = false;
	for(std::deque<superBlock>::iterator iter = superBuffer.begin(); iter != superBuffer.end(); iter++){
		if(iter->addBlock(atoi(blockNumber.c_str()), fileBlock)){
			blockAdded = true;
			break;
		}
	}

	if(!blockAdded){
		delete fileBlock;
	}
}

void download::calculateSpeed()
{
	time_t curr = time(0);

	std::ostringstream currentTime_s;
	currentTime_s << curr;

	int currentTime = atoi(currentTime_s.str().c_str());
	bool updated = false;

	//if the vectors are empty
	if(downloadSecond.empty()){
		downloadSecond.push_back(currentTime);
		secondBytes.push_back(global::BUFFER_SIZE);
		updated = true;
	}

	//get rid of information that's older than what to average
	if(currentTime - downloadSecond.front() >= averageSeconds){
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
	for(int x=1; x<downloadSecond.size()-1; x++){
		totalBytes += secondBytes.at(x);
	}

	//divide by the time
	downloadSpeed = totalBytes / (averageSeconds - 2);
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
	calculateSpeed();

	//create another superBlock if the buffer is empty and download not completed
	if(superBuffer.empty() && currentSuperBlock <= lastSuperBlock){
		superBlock SB(currentSuperBlock++, lastBlock);
		superBuffer.push_front(SB);
	}

	//write out the oldest superBlocks if they're complete
	while(superBuffer.back().complete()){

		writeSuperBlock(superBuffer.back().container);
		superBuffer.pop_back();

		if(superBuffer.empty()){
			break;
		}
	}

	/*
	If all requests were made but the superBlock is not complete start working
	on new superBlocks to give slower servers time to send their fileBlocks for
	the old superBlocks.
	*/
	if(superBuffer.front().allRequested() && !superBuffer.front().complete()){

		if(superBuffer.size() >= global::SUPERBUFFER_SIZE){
			blockCount = superBuffer.back().getRequest();
		}
		else{
			if(currentSuperBlock >= lastSuperBlock){
				blockCount = superBuffer.back().getRequest();
			}
			else{
				superBlock SB(currentSuperBlock++, lastBlock);
				superBuffer.push_front(SB);
				blockCount = superBuffer.front().getRequest();
			}
		}
	}
	else if(!superBuffer.front().allRequested()){
		blockCount = superBuffer.front().getRequest();
	}

	/*
	Don't return the last block number until it's the absolute last needed by
	this download. This makes checking for a completed download a lot easier!
	*/
	if(blockCount == lastBlock){

		//check if there are no superBlocks left but the last one
		if(superBuffer.size() == 1){
			int request = superBuffer.back().getRequest();

			//check if no more block requests are left but the last one
			if(request == -1){
				return lastBlock;
			}
			else{ //return a remaining request that isn't the last request
				return request;
			}
		}
		else{ //the superBuffer has more than one block in it, serve the oldest
			blockCount = superBuffer.back().getRequest();
			return blockCount;
		}
	}
	else{ //any request but the last one gets served here
		return blockCount;
	}
}

const int & download::getSpeed()
{
	return downloadSpeed;
}

bool download::hasSocket(int socketfd)
{
	for(std::list<serverElement>::iterator iter = Server.begin(); iter != Server.end(); iter++){
		if(iter->socketfd == socketfd){
			return true;
		}
	}

	return false;
}

void download::processBuffer(int socketfd, char recvBuff[], int nbytes)
{
	//fill the bucket
	for(std::list<serverElement>::iterator iter = Server.begin(); iter != Server.end(); iter++){

		//add the buffer to the right bucket
		if(iter->socketfd == socketfd){
			iter->bucket.append(recvBuff, nbytes);
#ifdef DEBUG_VERBOSE
			std::cout << "info: bucket.size(): " << iter->bucket.size() << std::endl;
#endif
		}

		//disconnect the server if it's being nasty!
		if(iter->bucket.size() > 3*global::BUFFER_SIZE){
#ifdef DEBUG
			std::cout << "error: client::processBuffer() detected buffer overrun from " << iter->server_IP << std::endl;
#endif

//DEBUG, add in something to disconnect sockets


			//invalidSockets.push_back(iter->socketfd);
			Server.erase(iter);
		}

		//if full block received write to tree and ready another request
		if(iter->bucket.size() % global::BUFFER_SIZE == 0){
			addBlock(iter->bucket);
			iter->ready = true;
		}

		if(iter->lastRequest){

			//check for zero in case the lastrequesting block: 1536
			if(iter->bucket.size() != 0){
				//add last partial block to tree
				if(iter->bucket.size() == lastBlockSize){
					addBlock(iter->bucket);

#ifdef UNRELIABLE_CLIENT
					/*
					This exists to make UNRELIABLE_CLIENT work. It wouldn't be needed
					under real conditions because if the program got to this point it
					wouldn't "lose" a fileBlock.
					*/
					if(!superBuffer.back().complete()){
						iter->ready = true;
					}
#endif
				}
				else{ //full block not yet received
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

void download::writeSuperBlock(std::string * container[])
{
#ifdef DEBUG
	std::cout << "download::writeSuperBlock() was called" << "\n";
#endif

	std::ofstream fout(filePath.c_str(), std::ios::app);

	if(fout.is_open()){

		for(int x=0; x<global::SUPERBLOCK_SIZE; x++){
			if(container[x] != 0){
				fout.write(container[x]->c_str(), container[x]->size());
			}
#ifdef DEBUG
			else{
				std::cout << "download::writeSuperBlock() is trying to write a incomplete superBlock" << std::endl;
			}
#endif
		}

		fout.close();
	}
	else{
#ifdef DEBUG
		std::cout << "error: download::writeTree() error opening file" << std::endl;
#endif
	}
}

