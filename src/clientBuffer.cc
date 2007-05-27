//std
#include <fstream>
#include <iostream>
#include <sstream>

#include "clientBuffer.h"

clientBuffer::clientBuffer()
{
	downloadComplete = false;

	averageSeconds = 3;
	downloadSpeed = 0;
	currentSuperBlock = 0;
}

int clientBuffer::addBlock(std::string & bucket)
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
	if(random < global::UNRELIABLE_CLIENT_PERCENT - 1){
		std::cout << "testing: client::addToTree(): OOPs! I dropped fileBlock " << fileBlock << std::endl;

		if(atoi(blockNumber.c_str()) == lastBlock){
			bucket.clear();
		}
		else{
			bucket = bucket.substr(global::BUFFER_SIZE);
		}

		return 0;
	}
#endif

	//prepare fileBlock to be added to a superBlock
	std::string * fileBlock = new std::string;
	if(bucket.size() >= global::BUFFER_SIZE){
		*fileBlock = bucket.substr(global::CONTROL_SIZE, global::BUFFER_SIZE - global::CONTROL_SIZE);
		bucket = bucket.substr(global::BUFFER_SIZE);
	}
	else{ //last block encountered
		*fileBlock = bucket.substr(global::CONTROL_SIZE, bucket.size() - global::CONTROL_SIZE);
		bucket.clear();
	}

	for(std::deque<superBlock>::iterator iter = superBuffer.begin(); iter != superBuffer.end(); iter++){
		if(iter->addBlock(atoi(blockNumber.c_str()), fileBlock)){
			break;
		}
	}
}

void clientBuffer::calculateSpeed()
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

bool clientBuffer::complete()
{
	return downloadComplete;
}

int clientBuffer::getRequest()
{
	calculateSpeed();

	//download just started, add the first superBlock
	if(currentSuperBlock == 0){
		superBlock SB(currentSuperBlock++, lastBlock);
		superBuffer.push_back(SB);
	}

	//write out the oldest superBlocks if they're complete
	while(superBuffer.back().complete()){

		writeSuperBlock(superBuffer.back().container);
		superBuffer.pop_back();

		if(superBuffer.empty()){
			break;
		}
	}	

	//create another superBlock if the buffer is empty and download not completed
	if(superBuffer.empty() && currentSuperBlock <= lastSuperBlock){
		superBlock SB(currentSuperBlock++, lastBlock);
		superBuffer.push_front(SB);
	}

	//save blockCount for percent complete calculation
	blockCount = superBuffer.front().getRequest();

	return blockCount;
}

bool clientBuffer::hasSocket(int socketfd)
{
	for(std::list<serverElement>::iterator iter = Server.begin(); iter != Server.end(); iter++){
		if(iter->socketfd == socketfd){
			return true;
		}
	}

	return false;
}

void clientBuffer::processBuffer(int socketfd, char recvBuff[], int nbytes)
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
			invalidSockets.push_back(iter->socketfd);
			Server.erase(iter);
		}

		//if full block received write to tree and ready another request
		if(iter->bucket.size() % global::BUFFER_SIZE == 0){
			addBlock(iter->bucket);
			iter->ready = true;
		}

		if(iter->lastRequest){

			//check for zero in case the last block was exactly BUFFER_SIZE
			if(iter->bucket.size() != 0){
				//add last partial block to tree
				if(iter->bucket.size() == lastBlockSize){
					addBlock(iter->bucket);
					iter->lastRequest = false;
				}
				else{ //full block not yet received
					break;
				}
			}

			//write out the oldest superBlocks if they're complete
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

void clientBuffer::writeSuperBlock(std::string * container[])
{
	std::ofstream fout(filePath.c_str(), std::ios::app);

	if(fout.is_open()){

		for(int x=0; x<global::SUPERBLOCK_SIZE; x++){
			if(container[x] != 0){
				fout.write(container[x]->c_str(), container[x]->size());
			}
		}

		fout.close();
	}
	else{
#ifdef DEBUG
		std::cout << "error: clientBuffer::writeTree() error opening file" << std::endl;
#endif
	}
}

