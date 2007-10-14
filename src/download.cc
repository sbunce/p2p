//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//std
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>

#include "download.h"

download::download(const std::string & hash_in, const std::string & fileName_in, 
	const std::string & filePath_in, const int & fileSize_in, const int & latestRequest_in,
	const int & lastBlock_in, const int & lastBlockSize_in, const int & lastSuperBlock_in,
	const int & currentSuperBlock_in, bool * downloadComplete_flag_in)
{
	//non-defaults
	hash = hash_in;
	fileName = fileName_in;
	filePath = filePath_in;
	fileSize = fileSize_in;
	latestRequest = latestRequest_in;
	lastBlock = lastBlock_in;
	lastBlockSize = lastBlockSize_in;
	lastSuperBlock = lastSuperBlock_in;
	currentSuperBlock = currentSuperBlock_in;
	downloadComplete_flag = downloadComplete_flag_in;

	//defaults
	downloadComplete = false;
	downloadSpeed = 0;

#ifdef UNRELIABLE_CLIENT
	std::srand(time(0));
#endif

	//set the hash type
	SHA.Init(global::HASH_TYPE);
}

void download::addBlock(const int & blockNumber, std::string & block)
{
	calculateSpeed();

#ifdef UNRELIABLE_CLIENT
	int random = rand() % 100;
	if(random < global::UNRELIABLE_CLIENT_VALUE){
		std::cout << "testing: client::addBlock(): OOPs! I dropped fileBlock " << blockNumber << "\n";
		if(blockNumber == lastBlock){
			block.clear();
		}
		else{
			block.erase(0, global::BUFFER_SIZE);
		}
	}
	else{
#endif

	//trim off protocol information
	block.erase(0, global::S_CTRL_SIZE);

	//add the block to the appropriate superBlock
	for(std::deque<superBlock>::iterator iter0 = superBuffer.begin(); iter0 != superBuffer.end(); ++iter0){
		if(iter0->addBlock(blockNumber, block)){
			break;
		}
	}
#ifdef UNRELIABLE_CLIENT
	}
#endif

	//the superBuffer may be empty if the last block is requested more than once
	if(blockNumber == lastBlock && !superBuffer.empty()){
		writeSuperBlock(superBuffer.back().container);
		superBuffer.pop_back();
		downloadComplete = true;
		*downloadComplete_flag = true;
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
	for(int x = 1; x <= downloadSecond.size() - 1; ++x){
		totalBytes += secondBytes[x];
	}

	//divide by the time
	downloadSpeed = totalBytes / global::SPEED_AVERAGE;
}

bool download::complete()
{
	return downloadComplete;
}

const int & download::getBytesExpected()
{
	if(latestRequest == lastBlock){
		return lastBlockSize;
	}
	else{
		return global::BUFFER_SIZE;
	}
}

const std::string & download::getFileName()
{
	return fileName;
}

const int & download::getFileSize()
{
	return fileSize;
}

const std::string & download::getHash()
{
	return hash;
}

std::list<std::string> & download::get_IPs()
{
	get_IPs_list.clear();

	std::list<connection>::iterator iter_cur, iter_end;
	iter_cur = currentConnections.begin();
	iter_end = currentConnections.end();
	while(iter_cur != iter_end){
		get_IPs_list.push_back(iter_cur->server_IP);
		++iter_cur;
	}

	return get_IPs_list;
}

const int & download::getLatestRequest()
{
	return latestRequest;
}

const int download::getRequest()
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
	 front() = back()
	-----------------------
	|*******************M*|
	-----------------------
	At this point a new leading SB would be added and that missing FB would be
	given time to arrive.
	*/
	if(superBuffer.empty()){ //P1
		superBuffer.push_front(superBlock(currentSuperBlock++, lastBlock));
		latestRequest = superBuffer.front().getRequest();
	}
	else if(superBuffer.front().allRequested()){ //P2
		if(superBuffer.size() < 2 && currentSuperBlock <= lastSuperBlock){
			superBuffer.push_front(superBlock(currentSuperBlock++, lastBlock));
		}
		latestRequest = superBuffer.front().getRequest();
	}
	else if(superBuffer.front().halfRequested()){ //P3
		latestRequest = superBuffer.back().getRequest();
	}
	else{ //P4
		latestRequest = superBuffer.front().getRequest();
	}

	/*
	Don't return the last block number until it's the absolute last needed by
	this download. This makes checking for a completed download a lot easier!
	*/
	if(latestRequest == lastBlock){

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
				latestRequest = request;
				return latestRequest;
			}
		}
		else{ //more than one superBlock, serve request from oldest
			latestRequest = superBuffer.back().getRequest();
			return latestRequest;
		}
	}
	else{ //any request but the last one gets returned without the above checking
		return latestRequest;
	}
}

const int & download::getSpeed()
{
	return downloadSpeed;
}

void download::regConnection(std::string & server_IP)
{
	connection temp(server_IP);
	currentConnections.push_back(temp);
}

void download::unregConnection(const std::string & server_IP)
{
	std::list<connection>::iterator iter_cur, iter_end;
	iter_cur = currentConnections.begin();
	iter_end = currentConnections.end();
	while(iter_cur != iter_end){
		if(server_IP == iter_cur->server_IP){
			currentConnections.erase(iter_cur);
			break;
		}
		++iter_cur;
	}
}

void download::stop()
{
	namespace fs = boost::filesystem;

	downloadComplete = true;
	superBuffer.clear();
	fs::path path = fs::system_complete(fs::path(filePath, fs::native));
	fs::remove(path);
	*downloadComplete_flag = true;
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

		for(int x=0; x<global::SUPERBLOCK_SIZE; ++x){
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

