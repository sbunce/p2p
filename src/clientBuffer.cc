//std
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>

#include "clientBuffer.h"

clientBuffer::clientBuffer()
{
	requestLast = false;
	averageSeconds = 3;
	downloadSpeed = 0;
	BF_offset = 0;
	ready = true;
	lowestRequest = 0;
	highestRequest = 0;

	//seed random number generator
	std::srand(time(0));
}

int clientBuffer::addToTree(std::string & bucket)
{
	//find position of delimiters
	int first = bucket.find_first_of(",");
	int second = bucket.find_first_of(",", first+1);
	int third = bucket.find_first_of(",", second+1);

	//get control data
	std::string command = bucket.substr(0, first);
	std::string file_ID = bucket.substr(first+1, (second - 1) - first);
	std::string fileBlock = bucket.substr(second+1, (third - 1) - second);

#ifdef UNRELIABLE_CLIENT
	//this exists for debug purposes, 1% chance of "dropping" a packet
	int random = rand() % 100;
	if(random < global::UNRELIABLE_CLIENT_PERCENT - 1){
		std::cout << "testing: client::addToTree(): OOPs! I dropped fileBlock " << fileBlock << std::endl;

		if(atoi(fileBlock.c_str()) == lastBlock){
			bucket.clear();
		}
		else{
			bucket = bucket.substr(global::BUFFER_SIZE);
		}

		return 0;
	}
#endif

	//check for a valid block received
	if((atoi(fileBlock.c_str()) < lowestRequest || atoi(fileBlock.c_str()) > highestRequest) && !requestLast){
#ifdef DEBUG_VERBOSE
		std::cout << "info: clientBuffer::addToTree() detected invalid block from server" << std::endl;
#endif
		//exit out, missing block will be located when the end of this superblock is reached
		return 0;
	}

	//prepare data to put in tree
	bst::treeNode * node = new bst::treeNode;
	node->key = atoi(fileBlock.c_str());
	if(bucket.size() >= global::BUFFER_SIZE){
		node->data = bucket.substr(global::CONTROL_SIZE, global::BUFFER_SIZE - global::CONTROL_SIZE);
		bucket = bucket.substr(global::BUFFER_SIZE);
	}
	else{ //last block encountered
		node->data = bucket.substr(global::CONTROL_SIZE, bucket.size() - global::CONTROL_SIZE);
		bucket.clear();
	}

	tree.addNode(node);
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

bool clientBuffer::completed()
{
	findMissing();

	if(request.empty()){ //download complete, no missing blocks
		//write the tree to file
		writeTree();

#ifdef DEBUG_VERBOSE
	std::cout << "info: clientBuffer::completed(): last super block complete" << std::endl;
#endif
		return true;
	}
	else{ //download not complete, fileBlock's missing in last superBlock
#ifdef DEBUG_VERBOSE
		std::cout << "info: clientBuffer::completed(): missing block(s) found in last super block" << std::endl;
#endif

		/*
		If missing blocks found the blockCount would have been decremented. Prepare
		to send another request by marking ready to be true.
		*/
		ready = true;
		return false;
	}
}

int clientBuffer::generateRequests()
{
	//fill buffer
	for(int x=(BF_offset * global::BUFFER_FACTOR); x<(BF_offset * global::BUFFER_FACTOR) + global::BUFFER_FACTOR; x++){
		//check to see if this is the last request buffer
		if(x == lastBlock){
			requestLast = true;
			break;
		}

		request.push_back(x);
	}

	//store the lowest and highest blocks
	//we will only accept blocks in this range in addToTree()
	lowestRequest = request.front();
	highestRequest = request.back();

	//randomize buffer
	int random, temp;
	for(int x=0; x<request.size(); x++){
		random = rand() % request.size();
		temp = request.at(random);
		request.at(random) = request.at(x);
		request.at(x) = temp;
	}

	//increment BF_offset for next generateRequests run
	BF_offset++;
}

void clientBuffer::findMissing()
{
	tree.findMissing(request, lowestRequest, highestRequest);

	//if no blocks missing this won't change blockCount
	blockCount -= request.size();
}

int clientBuffer::getBlockRequestNumber()
{
	if(request.empty()){
		findMissing();

		//request the last block if it's the only block left to request
		if(requestLast && request.empty()){
			request.push_back(lastBlock);
		}
		else{
			//if no missing requests generate new requests
			if(request.empty()){
				writeTree();
				generateRequests();
			}
		}
	}

	calculateSpeed();

	int blockNum = request.back();
	request.pop_back();
	return blockNum;
}

/*
This function will make sense when multi-source implemented.
*/
bool clientBuffer::hasSocket(int socketfd_in)
{
	return (socketfd_in == socketfd);
}

void clientBuffer::writeTree()
{
	std::ofstream fout(filePath.c_str(), std::ios::app);

	if(fout.is_open()){

		while(!tree.empty()){
			std::string * data = &tree.getLowestData();
			fout.write(data->c_str(), data->size());
			tree.deleteLastLowest();
		}

		fout.close();
	}
	else{
#ifdef DEBUG
		std::cout << "error: clientBuffer::writeTree() error opening file" << std::endl;
#endif
	}
}

