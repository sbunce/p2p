#include <iostream>

#include "super_block.h"

super_block::super_block(const int & super_block_number_in, const int & last_block_in)
{
	super_block_number = super_block_number_in;
	min_block = super_block_number * global::SUPERBLOCK_SIZE;
	max_block = (super_block_number * global::SUPERBLOCK_SIZE) + (global::SUPERBLOCK_SIZE - 1);

	next_request = min_block;

	if(max_block > last_block_in){
		max_block = last_block_in;
	}

	half_req = false;
}

bool super_block::add_block(const int & block_number, const std::string & file_block)
{
	if(block_number >= min_block && block_number <= max_block){
		if(container[block_number % global::SUPERBLOCK_SIZE].empty()){
			container[block_number % global::SUPERBLOCK_SIZE] = file_block;
		}
		return true;
	}
	else{
		return false;
	}
}

bool super_block::all_requested()
{
	/*
	After sequential requests are done linear searches are done for missing blocks.
	Because of this next_request stays one above max_block when all requests have 
	been returned once. Refer to get_request() to see why this works.
	*/
	return next_request > max_block;
}

bool super_block::complete()
{
	if(next_request > max_block){
		find_missing();
		return Missing_Blocks.empty();
	}
	else{
		return false;
	}
}

int super_block::get_request()
{
#ifdef REREQUEST_PERCENTAGE
	static long requestCount = 0;
	static long rerequestCount = 0;
#endif

	/*
	Return all request numbers in order. If that was already done then return
	missing block numbers in order.
	*/
	if(next_request <= max_block){

#ifdef REREQUEST_PERCENTAGE
		++requestCount;
#endif

		if((next_request % global::SUPERBLOCK_SIZE) == (max_block - min_block) / 2){
			half_req = true;
		}

		return next_request++;
	}
	else{

		find_missing();

		if(Missing_Blocks.empty()){
			return -1;
		}

		int block_number = Missing_Blocks.front();
		Missing_Blocks.pop_front();

#ifdef REREQUEST_PERCENTAGE
		rerequestCount++;
		static float percentage = ((float)rerequestCount / (float)requestCount) * 100;
		std::cout << "requestCount: " << requestCount << "\n";
		std::cout << "rerequestCount: " << rerequestCount << "\n";
		std::cout << "rerequest: " << percentage << "%\n";
#endif

		return block_number;
	}
}

void super_block::find_missing()
{
	Missing_Blocks.clear();

	for(int x=0; x<=max_block - min_block; ++x){
		if(container[x].empty()){
			 Missing_Blocks.push_back((super_block_number * global::SUPERBLOCK_SIZE) + x);
		}
	}
}

bool super_block::half_requested()
{
	return half_req;
}

