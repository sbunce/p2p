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
std::cout << "maxBlock: " << maxBlock << std::endl;
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

bool superBlock::complete()
{
	if(nextRequest > maxBlock){
		//only check for missing blocks after all blocks have been requested
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
#ifdef DEBUG
		if(missingBlocks.empty()){
			std::cout << "error: superBlock::getRequest() called when it needed no blocks" << std::endl;
		}
#endif

		int blockNumber = missingBlocks.front();
		missingBlocks.pop_front();
		return blockNumber;
	}
}

void superBlock::findMissing()
{
	for(int x=0; x<maxBlock - minBlock; x++){
		if(container[x] == 0){
			 missingBlocks.push_back((superBlockNumber * global::SUPERBLOCK_SIZE) + x);
		}
	}
}

