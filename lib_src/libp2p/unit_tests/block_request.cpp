//custom
#include "../block_request.hpp"

//include
#include <unit_test.hpp>

//standard
#include <cstdlib>

int fail(0);

void all_complete()
{
	boost::uint64_t block_count = 128;
	block_request BR(block_count);
	BR.approve_block_all();

	//add 8 hosts that each have all blocks (connection_ID 0 to 7)
	for(int x=0; x<8; ++x){
		bit_field BF;
		BR.download_reg(x, BF);
	}

	//we should be able to request all blocks
	int socket_FD = 0;
	while(!BR.complete()){
		int temp = socket_FD++ % 8;
		if(boost::optional<boost::uint64_t> block_num = BR.next_request(temp)){
			BR.add_block_local(temp, *block_num);
		}else{
			//exit immediately to avoid infinite loop on error
			LOG; exit(1);
		}
	}
}

void all_partial()
{
	boost::uint64_t block_count = 1024;
	block_request BR(block_count);
	BR.approve_block_all();

	//add 8 hosts that each have NO blocks (socket_FD 0 .. 7)
	for(int x=0; x<8; ++x){
		bit_field BF(block_count);
		BF = ~BF;
		BR.download_reg(x, BF);
	}

	//add blocks of different rarity
	std::srand(42);
	for(int x=0; x<block_count; ++x){
		//add a block to at least one remote host
		int host = std::rand() % 8;
		BR.add_block_remote(host, x);
		for(int y=0; y<8; ++y){
			//25% chance of adding a block
			if(std::rand() % 4 == 0){
				BR.add_block_remote(y, x);
			}
		}
	}

	//we should be able to request all blocks
	int socket_FD = 0;
	while(!BR.complete()){
		int temp = socket_FD++ % 8;
		if(boost::optional<boost::uint64_t> block_num = BR.next_request(temp)){
			BR.add_block_local(temp, *block_num);
		}
	}
}

int main()
{
	unit_test::timeout();

	all_complete();
	all_partial();
	return fail;
}
