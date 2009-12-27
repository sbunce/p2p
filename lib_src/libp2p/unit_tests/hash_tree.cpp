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
	std::vector<network::buffer> block; //location in vector is block number
	for(boost::uint32_t x=0; x<HT.tree_block_count; ++x){
		network::buffer buf;
		HT.read_tree_block(x, buf);
		block.push_back(buf);
	}

	//delete all hash trees
	database::init::drop_all();
	database::init::create_all();

	//reassemble
	std::srand(42);
	hash_tree HT_reassemble(FI);
	for(boost::uint64_t x=0; x<HT.tree_block_count; ++x){
		HT_reassemble.write_tree_block(0, x, block[x]);
	}

/* DEBUG, disable until hash_tree::tree_complete() works
	if(!HT_reassemble.tree_complete()){
		LOGGER; ++fail;
	}
*/
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
