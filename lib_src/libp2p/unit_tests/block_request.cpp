//custom
#include "../block_request.hpp"

//standard
#include <cstdlib>

void all_complete()
{
	boost::uint32_t block_count = 128;
	block_request BR(block_count);

	//add 8 hosts that each have all blocks (socket_FD 0 .. 7)
	for(int x=0; x<8; ++x){
		BR.add_host(x);
	}

	//we should be able to request all blocks
	int socket_FD = 0;
	while(!BR.complete()){
		boost::uint32_t block;
		if(!BR.next(socket_FD++ % 8, block)){
			LOGGER; exit(1);
		}
	}
}

void all_partial()
{
	boost::uint32_t block_count = 128;
	block_request BR(block_count);

	//add 8 hosts that each have NO blocks (socket_FD 0 .. 7)
	for(int x=0; x<8; ++x){
		boost::dynamic_bitset<> BS(block_count);
		BS = ~BS;
		BR.add_host(x, BS);
	}

	//add blocks of different rarity
	std::srand(42);
	for(int x=0; x<block_count; ++x){
		//add a block to at least one remote host
		int host = std::rand() % 8;
		BR.add_block(host, x);
		for(int y=0; y<8; ++y){
			//25% chance of adding a block
			int chance = std::rand() % 4;
			if(chance == 0){
				BR.add_block(y, x);
			}
		}
	}

	//we should be able to request all blocks
	int socket_FD = 0;
	while(!BR.complete()){
		boost::uint32_t block;
		if(BR.next(socket_FD++ % 8, block)){
			std::cout << block << " ";
		}else{
			std::cout << ". ";
		}
	}
}

int main()
{
	all_complete();
	all_partial();
}
