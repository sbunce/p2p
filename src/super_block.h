#ifndef H_SUPERBLOCK
#define H_SUPERBLOCK

#include <deque>
#include <string>

#include "global.h"

class super_block
{
public:
	//holds the fileBlocks
	std::string container[global::SUPERBLOCK_SIZE];

	/*
	The super_block_number represents a group of fileBlocks which compose the
	super_block, it's used to calculate what fileBlocks the super_block needs.
	*/
	super_block(const int & super_block_number_in, const int & last_block_in);

	/*
	add_block      - returns false if super_block doesn't need the block, otherwise adds it
	all_requested  - all blocks have been requested(but not all necessarily received)
	complete      - returns true if the super_block is complete
	get_request    - returns the number of a file_block this super_block needs
	half_requested - returns true if half of the blocks have been requested
	*/
	bool add_block(const int & block_number, const std::string & file_block);
	bool all_requested();
	bool complete();
	int get_request();
	bool half_requested();

private:
	//what super_block this class represents
	int super_block_number;

	//true when half of the fileBlocks have been requested
	bool half_req;

	/*
	min_block = super_block_number * SUPERBLOCK_SIZE
	max_block = ( super_block_number * SUPERBLOCK_SIZE ) + ( SUPERBLOCK_SIZE - 1 )
	*/
	int min_block; //lowest file_block this super_block needs
	int max_block; //highest file_block this super_block needs

	/*
	This number is returned and incremented by getBlockNumber. When next_request
	is equal to max_block a linear search through container will be done by 
	find_missing() to search for missing fileBlocks.
	*/
	int next_request;

	//stores missing blocks find_missing locates
	std::deque<int> Missing_Blocks;

	/*
	missing - finds missing blocks, not very expensive(CPU time) to call
	*/
	void find_missing();
};
#endif
