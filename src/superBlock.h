#ifndef H_SUPERBLOCK
#define H_SUPERBLOCK

#include <deque>
#include <string>

#include "global.h"

class superBlock
{
public:
	//holds the fileBlocks
	std::string container[global::SUPERBLOCK_SIZE];

	/*
	The superBlockNumber represents a group of fileBlocks which compose the
	superBlock, it's used to calculate what fileBlocks the superBlock needs.
	*/
	superBlock(const int & superBlockNumber_in, const int & lastBlock_in);

	/*
	add_block      - returns false if superBlock doesn't need the block, otherwise adds it
	allRequested  - all blocks have been requested(but not all necessarily received)
	complete      - returns true if the superBlock is complete
	get_request    - returns the number of a fileBlock this superBlock needs
	halfRequested - returns true if half of the blocks have been requested
	*/
	bool add_block(const int & blockNumber, const std::string & fileBlock);
	bool allRequested();
	bool complete();
	int get_request();
	bool halfRequested();

private:
	//what superBlock this class represents
	int superBlockNumber;

	//true when half of the fileBlocks have been requested
	bool halfReq;

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
