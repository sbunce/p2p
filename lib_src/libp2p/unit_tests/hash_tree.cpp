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

int fail(0);

//create test files to hash if they don't already exist
void create_test_file(const file_info & FI)
{
	std::fstream fout(FI.path.get().c_str(), std::ios::out | std::ios::binary
		| std::ios::trunc);
	std::srand(42);
	for(boost::uint64_t x=0; x<FI.file_size; ++x){
		char ch = (char)std::rand() % 255;
		fout.put(ch);
	}
	fout.flush();
}

//randomly reassemble a hash tree
void test(const unsigned size)
{
	//delete all hash trees
	database::init::drop_all();
	database::init::create_all();

	//setup test file
	file_info FI;
	std::stringstream ss;
	ss << path::share() << size << "_block";
	FI.path = ss.str();
	FI.file_size = size * protocol::file_block_size;
	create_test_file(FI);

	//create hash tree and check it
	hash_tree HT(FI);
	if(HT.create() != hash_tree::good){
		LOGGER; ++fail;
	}
	FI.hash = HT.hash;

	//read all blocks from the hash tree in to memory
	std::vector<std::string> block; //location in vector is block number
	for(boost::uint32_t x=0; x<HT.tree_block_count; ++x){
		std::string b;
		HT.read_block(x, b);
		block.push_back(b);
	}

	//delete all hash trees
	database::init::drop_all();
	database::init::create_all();

	//random reassemble with 50% corrupt blocks
	std::srand(42);
	hash_tree HT_reassemble(FI);
	for(boost::uint64_t x=0; x<HT.tree_block_count; ++x){
		//always exclude first block for the small tests
		if(x != 0 && std::rand() % 2 != 0){
			HT_reassemble.write_block(0, x, block[x]);
		}
	}

	//should not be complete since we corrupted a block
	if(HT_reassemble.complete()){
		LOGGER; ++fail;
	}
return;
/* DISABLE
	//replace the corrupted blocks, add one host that has all blocks
	HT_reassemble.Block_Request.add_host_complete(0);
	while(!HT_reassemble.complete()){
		boost::uint64_t block_num;
		if(!HT_reassemble.Block_Request.next_request(0, block_num)){
			LOGGER; ++fail;
		}
		HT_reassemble.write_block(0, block_num, block[block_num]);
	}
*/

	//now the tree should be complete
	if(!HT_reassemble.complete()){
		LOGGER; ++fail;
	}
}

int main()
{
	//setup database and make sure hash table clear
	path::unit_test_override("hash_tree.db");
	path::create_required_directories();

	test(2);
	test(3);
	test(4);
	test(256);
	test(257);
	test(1024);
	test(1536);

	//remove temporary files
	path::remove_temporary_hash_tree_files();
	return fail;
}
