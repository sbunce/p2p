//custom
#include "../database.hpp"
#include "../hash_tree.hpp"
#include "../path.hpp"
#include "../protocol.hpp"
#include "../share.hpp"

//include
#include <logger.hpp>

//standard
#include <algorithm>

//create test files to hash if they don't already exist
void create_test_file(const file_info & FI)
{
	std::fstream fout(FI.path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	for(int x=0; x<FI.file_size; ++x){
		if(x % 80 == 0 && x != 0){
			fout.put('\n');
		}else{
			fout.put('0');
		}
	}
	fout.flush();
}

//create hash trees and check them
void create_and_check(const unsigned size)
{
	file_info FI;
	std::stringstream ss;
	ss << path::share() << size << "_block";
	FI.path = ss.str();
	FI.file_size = size * protocol::FILE_BLOCK_SIZE;
	create_test_file(FI);
	if(hash_tree::create(FI) != hash_tree::good){
		LOGGER; exit(1);
	}
	hash_tree HT(FI);
	HT.check();
	if(!HT.complete()){
		LOGGER; exit(1);
	}
}

int rand_func(int n)
{
    return std::rand() % n;
}

//randomly reassemble a hash tree
void random_assemble()
{
	unsigned size = 1536;

	file_info FI;
	std::stringstream ss;
	ss << path::share() << size << "_block";
	FI.path = ss.str();
	FI.file_size = size * protocol::FILE_BLOCK_SIZE;
	create_test_file(FI);
	if(hash_tree::create(FI) != hash_tree::good){
		LOGGER; exit(1);
	}
	hash_tree HT(FI);
	HT.check();
	if(!HT.complete()){
		LOGGER; exit(1);
	}

	//read all blocks from the source hash tree
	std::vector<std::string> block; //location in vector is block number
	for(boost::uint32_t x=0; x<HT.tree_block_count; ++x){
		std::string B;
		HT.read_block(x, B);
		block.push_back(B);
	}

	//make a vector of all block numbers and randomize it
	std::vector<unsigned> block_number;
	for(unsigned x=0; x<block.size(); ++x){
		block_number.push_back(x);
	}
	std::srand(42);
	std::random_shuffle(block_number.begin(), block_number.end(), rand_func);

	//"corrupt" every other block in invertible way so we can "uncorrupt" later
	bool alternator = false;
	for(unsigned x=0; x<block.size(); ++x){
		if(alternator){
			(*block.begin())[x] = ~(*block.begin())[x];
		}
		alternator = !alternator;
	}

	//reassemble hash tree in random order
	hash_tree HT_reassemble(FI);
	for(boost::uint64_t x=0; x<HT.tree_block_count; ++x){
		HT_reassemble.write_block(block_number[x], block[block_number[x]], "127.0.0.1");
	}

	//"uncorrupt"
	alternator = false;
	for(unsigned x=0; x<block.size(); ++x){
		if(alternator){
			(*block.begin())[x] = ~(*block.begin())[x];
		}
		alternator = !alternator;
	}

	//should not be complete since we corrupted a block
	if(HT_reassemble.complete()){
		LOGGER; exit(1);
	}

	//replace the corrupted block, add one host that has all blocks
	HT_reassemble.Block_Request.add_host(0);
	while(!HT_reassemble.complete()){
		boost::uint32_t num;
		if(!HT_reassemble.Block_Request.next(0, num)){
			LOGGER; exit(1);
		}
		HT_reassemble.write_block(num, block[num], "127.0.0.1");
	}

	//now the tree should be complete
	if(!HT_reassemble.complete()){
		LOGGER; exit(1);
	}
}

int main()
{
	//setup database and make sure hash table clear
	path::unit_test_override("hash_tree.db");
	database::init::drop_all();
	database::init::create_all();

	//create required directories for the shared files and temp files
	path::create_required_directories();

	create_and_check(1);
	create_and_check(2);
	create_and_check(3);
	create_and_check(4);
	create_and_check(256);
	create_and_check(257);
	create_and_check(1024);

	random_assemble();

	//remove temporary files
	path::remove_temporary_hash_tree_files();
}
