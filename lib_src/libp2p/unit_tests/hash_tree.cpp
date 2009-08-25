//custom
#include "../database.hpp"
#include "../hash_tree.hpp"
#include "../path.hpp"
#include "../protocol.hpp"

//include
#include <logger.hpp>

//standard
#include <algorithm>

//create test files to hash if they don't already exist
void create_test_file(const std::string & name, const unsigned & bytes)
{
	std::fstream f((path::share() + name).c_str(), std::ios::out
		| std::ios::binary | std::ios::trunc);
	for(int x=0; x<bytes; ++x){
		if(x % 80 == 0 && x != 0){
			f.put('\n');
		}else{
			f.put('0');
		}
	}
	f.flush();
}

//create hash trees and check them
void create_and_check()
{
	std::string root_hash;

	//test 1
	if(!hash_tree::create(path::share() + "1_block", 1 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_1(root_hash, 1 * protocol::FILE_BLOCK_SIZE);
	if(!HT_1.complete()){
		LOGGER; exit(1);
	}

	//test 2
	if(!hash_tree::create(path::share() + "2_block", 2 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_2(root_hash, 2 * protocol::FILE_BLOCK_SIZE);
	HT_2.check();
	if(!HT_2.complete()){
		LOGGER; exit(1);
	}

	//test 3
	if(!hash_tree::create(path::share() + "3_block", 3 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_3(root_hash, 3 * protocol::FILE_BLOCK_SIZE);
	HT_3.check();
	if(!HT_3.complete()){
		LOGGER; exit(1);
	}

	//test 4
	if(!hash_tree::create(path::share() + "4_block", 4 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_4(root_hash, 4 * protocol::FILE_BLOCK_SIZE);
	HT_4.check();
	if(!HT_4.complete()){
		LOGGER; exit(1);
	}

	//test 5
	if(!hash_tree::create(path::share() + "256_block", 256 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_256(root_hash, 256 * protocol::FILE_BLOCK_SIZE);
	HT_256.check();
	if(!HT_256.complete()){
		LOGGER; exit(1);
	}

	//test 6
	if(!hash_tree::create(path::share() + "257_block", 257 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_257(root_hash, 257 * protocol::FILE_BLOCK_SIZE);
	HT_257.check();
	if(!HT_257.complete()){
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
	std::string root_hash;
	if(!hash_tree::create(path::share() + "1024_block", 1024 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_1024(root_hash, 1024 * protocol::FILE_BLOCK_SIZE);
	HT_1024.check();
	if(!HT_1024.complete()){
		LOGGER; exit(1);
	}

	//read all blocks from the source hash tree then delete source tree
	std::vector<std::string> block; //location in vector is block number
	for(boost::uint32_t x=0; x<HT_1024.tree_block_count; ++x){
		std::string B;
		HT_1024.read_block(x, B);
		block.push_back(B);
	}
	database::table::hash::delete_tree(HT_1024.hash);

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
	hash_tree HT_reassemble(HT_1024.hash, HT_1024.file_size);
	for(boost::uint64_t x=0; x<HT_1024.tree_block_count; ++x){
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

	//create test files to hash
	create_test_file("1_block", 1 * protocol::FILE_BLOCK_SIZE);
	create_test_file("2_block", 2 * protocol::FILE_BLOCK_SIZE);
	create_test_file("3_block", 3 * protocol::FILE_BLOCK_SIZE);
	create_test_file("4_block", 4 * protocol::FILE_BLOCK_SIZE);
	create_test_file("256_block", 256 * protocol::FILE_BLOCK_SIZE);
	create_test_file("257_block", 257 * protocol::FILE_BLOCK_SIZE);
	create_test_file("1024_block", 1024 * protocol::FILE_BLOCK_SIZE);

	create_and_check();
	random_assemble();

	//remove temporary files
	path::remove_temporary_hash_tree_files();
}
