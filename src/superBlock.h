#ifndef H_SUPERBLOCK
#define H_SUPERBLOCK

#include <deque>
#include <string>

#include "globals.h"

class superBlock
{
public:
	//holds the fileBlocks
	std::string * container[global::SUPERBLOCK_SIZE];

	/*
	The superBlockNumber represents a group of fileBlocks which compose the
	superBlock, it's used to calculate what fileBlocks the superBlock needs.
	*/
	superBlock(int superBlockNumber_in, int lastBlock_in);
	~superBlock();

	/*
	addBlock     - returns false if superBlock doesn't need the block, otherwise adds it
	allRequested - all blocks have been requested(but not all necessarily received)
	complete     - returns true if the superBlock is complete
	getRequest   - returns the number of a fileBlock this superBlock needs
	*/
	bool addBlock(int blockNumber, std::string * fileBlock);
	bool allRequested();
	bool complete();
	int getRequest();

private:
	int superBlockNumber; //what superBlock this class represents

	/*
	minBlock = superBlockNumber * SUPERBLOCK_SIZE
	maxBlock = ( superBlockNumber * SUPERBLOCK_SIZE ) + ( SUPERBLOCK_SIZE - 1 )
	*/
	int minBlock; //lowest fileBlock this superBlock needs
	int maxBlock; //highest fileBlock this superBlock needs

	/*
	This number is returned and incremented by getBlockNumber. When nextRequest
	is equal to maxBlock a linear search through container will be done by 
	findMissing() to search for missing fileBlocks.
	*/
	int nextRequest;

	//stores missing blocks findMissing locates
	std::deque<int> missingBlocks;

	/*
	missing - finds missing blocks, not very expensive(CPU time) to call
	*/
	void findMissing();

};
#endif
