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
	std::fstream f((path::share() + name).c_str(), std::ios::in);
	if(f.is_open()){
		return;
	}

	f.open((path::share() + name).c_str(), std::ios::out | std::ios::binary);
	f.clear();
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
	boost::uint64_t bad_block;

	//test 1
	if(!hash_tree::create(path::share() + "1_block", 1 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_1(root_hash, 1 * protocol::FILE_BLOCK_SIZE);
	if(HT_1.check(bad_block) != hash_tree::GOOD){
		LOGGER; exit(1);
	}

	//test 2
	if(!hash_tree::create(path::share() + "2_block", 2 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_2(root_hash, 2 * protocol::FILE_BLOCK_SIZE);
	if(HT_2.check(bad_block) != hash_tree::GOOD){
		LOGGER; exit(1);
	}

	//test 3
	if(!hash_tree::create(path::share() + "3_block", 3 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_3(root_hash, 3 * protocol::FILE_BLOCK_SIZE);
	if(HT_3.check(bad_block) != hash_tree::GOOD){
		LOGGER; exit(1);
	}

	//test 4
	if(!hash_tree::create(path::share() + "4_block", 4 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_4(root_hash, 4 * protocol::FILE_BLOCK_SIZE);
	if(HT_4.check(bad_block) != hash_tree::GOOD){
		LOGGER; exit(1);
	}

	//test 5
	if(!hash_tree::create(path::share() + "256_block", 256 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_256(root_hash, 256 * protocol::FILE_BLOCK_SIZE);
	if(HT_256.check(bad_block) != hash_tree::GOOD){
		LOGGER; exit(1);
	}

	//test 6
	if(!hash_tree::create(path::share() + "257_block", 257 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_257(root_hash, 257 * protocol::FILE_BLOCK_SIZE);
	if(HT_257.check(bad_block) != hash_tree::GOOD){
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
	boost::uint64_t bad_block;

	if(!hash_tree::create(path::share() + "1024_block", 1024 * protocol::FILE_BLOCK_SIZE, root_hash)){
		LOGGER; exit(1);
	}
	hash_tree HT_1024(root_hash, 1024 * protocol::FILE_BLOCK_SIZE);
	if(HT_1024.check(bad_block) != hash_tree::GOOD){
		LOGGER; exit(1);
	}

	//std::pair<block number, block>
	std::vector<std::pair<boost::uint64_t, std::string> > block;
	for(boost::uint64_t x=0; x<HT_1024.tree_block_count; ++x){
		std::pair<boost::uint64_t, std::string> B(x, std::string());
		HT_1024.read_block(B.first, B.second);
		block.push_back(B);
	}

	//deterministically randomize blocks
	std::srand(42);
	std::random_shuffle(block.begin(), block.end(), rand_func);

	//reassemble hash tree
	database::table::hash::delete_tree(HT_1024.root_hash, HT_1024.tree_size);
	hash_tree HT_reassemble(HT_1024.root_hash, HT_1024.file_size);
	for(boost::uint64_t x=0; x<HT_1024.tree_block_count; ++x){
		HT_reassemble.write_block(block[x].first, block[x].second, "127.0.0.1");
	}

	//check reassembled hash tree
	if(HT_reassemble.check(bad_block) != hash_tree::GOOD){
		LOGGER; exit(1);
	}
}

int main()
{
	//setup database and make sure hash table clear
	database::pool::singleton().unit_test_override("hash_tree.db");
	database::init init;
	database::table::hash::clear();

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
