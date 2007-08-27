//std
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>

#include "download.h"

download::download(const std::string & hash_in, const std::string & fileName_in, 
	const std::string & filePath_in, const int & fileSize_in, const int & blockCount_in,
	const int & lastBlock_in, const int & lastBlockSize_in, const int & lastSuperBlock_in,
	const int & currentSuperBlock_in)
{
	//non-defaults
	hash = hash_in;
	fileName = fileName_in;
	filePath = filePath_in;
	fileSize = fileSize_in;
	blockCount = blockCount_in;
	lastBlock = lastBlock_in;
	lastBlockSize = lastBlockSize_in;
	lastSuperBlock = lastSuperBlock_in;
	currentSuperBlock = currentSuperBlock_in;

	//defaults
	terminateDownload = false;
	downloadComplete = false;
	downloadSpeed = 0;

	//seed random number generator
	std::srand(time(0));

	//set the hash type
	SHA.Init(global::HASH_TYPE);
}

int download::addBlock(const int & blockNumber, std::string & bucket)
{
	calculateSpeed();

#ifdef UNRELIABLE_CLIENT
	int random = rand() % 100;
	if(random < global::UNRELIABLE_CLIENT_VALUE){
		std::cout << "testing: client::addToTree(): OOPs! I dropped fileBlock " << blockNumber << "\n";

		if(blockNumber == lastBlock){
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
		fileBlock = bucket.substr(global::S_CTRL_SIZE, global::BUFFER_SIZE - global::S_CTRL_SIZE);
		bucket.clear();
	}
	else{ //last block encountered
		fileBlock = bucket.substr(global::S_CTRL_SIZE, bucket.size() - global::S_CTRL_SIZE);
		bucket.clear();
	}

	for(std::deque<superBlock>::iterator iter = superBuffer.begin(); iter != superBuffer.end(); iter++){
		if(iter->addBlock(blockNumber, fileBlock)){
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

const int & download::getLastBlockSize()
{
	return lastBlockSize;
}

const std::string & download::getHash()
{
	return hash;
}

int download::getRequest()
{
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
	SB = superBlock
	FB = fileBlock
	* = requested fileBlock
	M = missing fileBlock

	Scenario #1 Ideal Conditions:
		P1 would load a SB and return the first request. P4 would serve all requests
	until SB was half completed. P3 would serve all requests until all requests
	made. At this point either P1 or P2 will get called depending on whether
	all FB were received for SB. This cycle would repeat under ideal conditions.

	Scenario #2 Unreliable Hosts:
		P1 would load a SB and return the first request. P4 would serve all requests
	until SB was half completed. P3 would serve all requests until all requests
	made. At this point we're missing FB that slow servers havn't sent yet.
	P2 adds a leading SB to the buffer. P4 requests FB for the leading SB until
	half the FB are requested. At this point P3 would rerequest FB from the back
	until those blocks were gotten. The back SB would be written as soon as it
	completed and P3 would start serving requests from the one remaining SB.

	Example of a rerequest:
	 back()                     front()
	-----------------------	   -----------------------
	|***M*******M*********| 	|***M******           |
	-----------------------   	-----------------------
	At this point the missing blocks from the back() would be rerequested.

	Example of new SB creation.
	 front() & back()
	-----------------------
	|*******************M*|
	-----------------------
	At this point a new leading SB would be added and that missing FB would be
	given time to arrive.
	*/
	if(superBuffer.empty()){ //P1
		superBlock SB(currentSuperBlock++, lastBlock);
		superBuffer.push_front(SB);
		blockCount = superBuffer.front().getRequest();
	}
	else if(superBuffer.front().allRequested()){ //P2
		if(superBuffer.size() < 2 && currentSuperBlock <= lastSuperBlock){
			superBlock SB(currentSuperBlock++, lastBlock);
			superBuffer.push_front(SB);
		}
		blockCount = superBuffer.front().getRequest();
	}
	else if(superBuffer.front().halfRequested()){ //P3
		blockCount = superBuffer.back().getRequest();
	}
	else{ //P4
		blockCount = superBuffer.front().getRequest();
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
		else{ //more than one superBlock, serve request from oldest
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

bool download::hasSocket(const int & socketfd, const int & nbytes)
{
	for(std::list<serverElement *>::iterator iter0 = Server.begin(); iter0 != Server.end(); iter0++){

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

void download::processBuffer(const int & socketfd, char recvBuff[], const int & nbytes)
{
	//fill the bucket
	for(std::list<serverElement *>::iterator iter0 = Server.begin(); iter0 != Server.end(); iter0++){

		//add the buffer to the right bucket and process
		if((*iter0)->socketfd == socketfd){

			(*iter0)->bucket.append(recvBuff, nbytes);
			(*iter0)->bytesExpected -= nbytes;

#ifdef DEBUG_VERBOSE
			std::cout << "info: download::processBuffer() IP: " << (*iter0)->server_IP << " bucket size: " << (*iter0)->bucket.size() << "\n";
			std::cout << "info: superBuffer.size(): " << superBuffer.size() << "\n";
#endif

#ifdef ABUSIVE_SERVER
			static int counter = 0;
			counter++;
			if(counter % global::ABUSIVE_SERVER_VALUE == 0){
				std::cout << "testing: client::processBuffer() simulated abuse from " << (*iter0)->server_IP << "\n";

				//overflow the buffer by one byte
				(*iter0)->bucket += " ";
			}
#endif

			//disconnect the server if it's being nasty!
			if((*iter0)->bucket.size() > global::BUFFER_SIZE){
#ifdef DEBUG
				std::cout << "error: client::processBuffer() detected buffer overrun from " << (*iter0)->server_IP << "\n";
#endif
				abusiveServerElement temp;
				temp.server_IP = (*iter0)->server_IP;
				temp.socketfd = (*iter0)->socketfd;
				abusiveServer.push_back(temp);

				break;
			}

			//if full block received add it to a superBlock and ready another request
			if((*iter0)->bucket.size() % global::BUFFER_SIZE == 0){
				addBlock((*iter0)->blockRequested, (*iter0)->bucket);
				(*iter0)->bytesExpected = 0;

				//only give up the token if the download is not terminating
				if(!terminateDownload){
					(*iter0)->token = false;
				}
			}

			//!complete() because often the last fileBlock gets requested more than once
			if((*iter0)->lastRequest && !complete()){

				//check for zero in case the last requested block was exactly global::BUFFER_SIZE
				if((*iter0)->bucket.size() != 0){

					//add last partial block to tree
					if((*iter0)->bucket.size() == lastBlockSize){
						addBlock((*iter0)->blockRequested, (*iter0)->bucket);

#ifdef UNRELIABLE_CLIENT
						/*
						This exists to make UNRELIABLE_CLIENT work. It wouldn't be needed
						under real conditions because if the program got to this point it
						wouldn't "drop" a fileBlock.
						*/
						if(!superBuffer.back().complete()){
							(*iter0)->bytesExpected = 0;

							//only give up the token if the download is not terminating
							if(!terminateDownload){
								(*iter0)->token = false;
							}
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
/*
	//get the messageDigest of the superBlock
	for(int x=0; x<global::SUPERBLOCK_SIZE; x++){
		SHA.Update((const sha_byte *) container[x].c_str(), container[x].size());
	}
	SHA.End();
	std::cout << SHA.StringHash() << "\n";
*/
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

void download::startTerminate()
{
	terminateDownload = true;
}

bool download::terminating()
{
	return terminateDownload;
}

bool download::readyTerminate()
{
	if(terminateDownload){
		for(std::list<serverElement *>::iterator iter0 = Server.begin(); iter0 != Server.end(); iter0++){

			if((*iter0)->bytesExpected != 0){
				return false;
			}
		}

		return true;
	}
	else{
		return false;
	}
}

void download::zeroSpeed()
{
	downloadSpeed = 0;
}

