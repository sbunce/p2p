#include <iostream>

#include "superBlock.h"

superBlock::superBlock(const int & superBlockNumber_in, const int & lastBlock_in)
{
	superBlockNumber = superBlockNumber_in;
	minBlock = superBlockNumber * global::SUPERBLOCK_SIZE;
	maxBlock = (superBlockNumber * global::SUPERBLOCK_SIZE) + (global::SUPERBLOCK_SIZE - 1);

	nextRequest = minBlock;

	if(maxBlock > lastBlock_in){
		maxBlock = lastBlock_in;
	}
}

bool superBlock::addBlock(const int & blockNumber, const std::string & fileBlock)
{
	if(blockNumber >= minBlock && blockNumber <= maxBlock){

		if(container[blockNumber % global::SUPERBLOCK_SIZE].empty()){
			container[blockNumber % global::SUPERBLOCK_SIZE] = fileBlock;
		}

		return true;
	}
	else{
		return false;
	}
}

bool superBlock::allRequested()
{
	/*
	After sequential requests are done linear searches are done for missing blocks.
	Because of this nextRequest stays one above maxBlock when all requests have 
	been returned once. Refer to getRequest() to see why this works.
	*/
	return nextRequest > maxBlock;
}

bool superBlock::complete()
{
	if(nextRequest > maxBlock){
		findMissing();
		return missingBlocks.empty();
	}
	else{
		return false;
	}
}

int superBlock::getRequest()
{
#ifdef REREQUEST_PERCENTAGE
	static long requestCount = 0;
	static long rerequestCount = 0;
#endif

	/*
	Return all request numbers in order. If that was already done then return
	missing block numbers in order.
	*/
	if(nextRequest <= maxBlock){
#ifdef REREQUEST_PERCENTAGE
		requestCount++;
#endif
		return nextRequest++;
	}
	else{

		findMissing();

		if(missingBlocks.empty()){
			return -1;
		}

#ifdef REREQUEST_PERCENTAGE
		rerequestCount++;
		static float percentage = ((float)rerequestCount / (float)requestCount) * 100;
		std::cout << "rerequest: " << percentage << "%\n";
		std::cout << "requestCount: " << requestCount << "\n";
		std::cout << "rerequestCount: " << rerequestCount << "\n";
#endif

		int blockNumber = missingBlocks.front();
		missingBlocks.pop_front();
		return blockNumber;
	}
}

void superBlock::findMissing()
{
	/*
	Don't check for missing blocks if missing blocks are already known. This way
	duplicates in missingBlocks don't have to be checked for.
	*/
	if(missingBlocks.empty()){
		for(int x=0; x<=maxBlock - minBlock; x++){
			if(container[x].empty()){
				 missingBlocks.push_back((superBlockNumber * global::SUPERBLOCK_SIZE) + x);
			}
		}
	}
}

