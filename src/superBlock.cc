#include <iostream>

#include "superBlock.h"

superBlock::superBlock(int superBlockNumber_in, int lastBlock_in)
{
	superBlockNumber = superBlockNumber_in;
	minBlock = superBlockNumber * global::SUPERBLOCK_SIZE;
	maxBlock = (superBlockNumber * global::SUPERBLOCK_SIZE) + (global::SUPERBLOCK_SIZE - 1);

	//set null pointers
	for(int x=0; x<global::SUPERBLOCK_SIZE; x++){
		container[x] = 0;
	}

	nextRequest = minBlock;

	if(maxBlock > lastBlock_in){
		maxBlock = lastBlock_in;
	}
}

superBlock::~superBlock()
{
	for(int x=0; x<global::SUPERBLOCK_SIZE; x++){
		if(container[x] != 0){
			delete container[x];
		}
	}
}

bool superBlock::addBlock(int blockNumber, std::string * fileBlock)
{
	if(blockNumber >= minBlock && blockNumber <= maxBlock){

		if(container[blockNumber % global::SUPERBLOCK_SIZE] == 0){
			container[blockNumber % global::SUPERBLOCK_SIZE] = fileBlock;
		}
		else{ //already have this block
			delete fileBlock;
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
	/*
	Return all request numbers in order. If that was already done then return
	missing block numbers in order.
	*/
	if(nextRequest <= maxBlock){
		return nextRequest++;
	}
	else{

		findMissing();

		if(missingBlocks.empty()){
			return -1;
		}

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
			if(container[x] == 0){
				 missingBlocks.push_back((superBlockNumber * global::SUPERBLOCK_SIZE) + x);
			}
		}
	}
}

