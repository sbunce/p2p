//custom
#include "../database.hpp"
#include "../hash_tree.hpp"
#include "../path.hpp"
#include "../protocol_tcp.hpp"
#include "../share.hpp"

//include
#include <logger.hpp>

//standard
#include <algorithm>

int fail(0);

//create test files to hash if they don't already exist
void create_file(const file_info & FI)
{
	std::fstream fout(FI.path.get().c_str(), std::ios::out | std::ios::binary
		| std::ios::trunc);
	unsigned data = 0;
	for(boost::uint64_t x=0; x<FI.file_size; ++x){
		char ch = (char)(data++ % 255);
		fout.put(ch);
	}
	fout.flush();
}

void create_reassemble(const unsigned size)
{
	database::init::drop_all();
	database::init::create_all();

	//setup test file
	file_info FI;
	std::stringstream ss;
	ss << path::share() << size << "_block";
	FI.path = ss.str();
	FI.file_size = size * protocol_tcp::file_block_size;
	create_file(FI);

	//create hash tree and check it
	hash_tree HT(FI);
	if(HT.create() != hash_tree::good){
		LOGGER; ++fail;
		return;
	}
	FI.hash = HT.hash;

	//hash tree should be good
	if(HT.check() != hash_tree::good){
		LOGGER; ++fail;
		return;
	}

	//read all blocks from the hash tree in to memory
	std::vector<network::buffer> block; //location in vector is block number
	for(boost::uint32_t x=0; x<HT.tree_block_count; ++x){
		network::buffer buf;
		HT.read_block(x, buf);
		block.push_back(buf);
	}

	database::init::drop_all();
	database::init::create_all();

	//reassemble
	hash_tree HT_reassemble(FI);
	for(boost::uint64_t x=0; x<HT.tree_block_count; ++x){
		HT_reassemble.write_block(x, block[x]);
	}

	//hash tree should be good
	if(HT_reassemble.check() != hash_tree::good){
		LOGGER; ++fail;
		return;
	}

	//check file blocks with rebuilt tree
	network::buffer buf;
	std::fstream fin(FI.path.get().c_str(), std::ios::in | std::ios::binary);
	if(!fin.good()){
		LOGGER; ++fail;
		return;
	}
	for(boost::uint64_t x=0; x<HT_reassemble.file_block_count; ++x){
		buf.reserve(protocol_tcp::file_block_size);
		fin.read(reinterpret_cast<char *>(buf.data()), protocol_tcp::file_block_size);
		if(!fin.good()){
			LOGGER; ++fail;
			return;
		}
		buf.resize(fin.gcount());
		if(HT_reassemble.check_file_block(x, buf) != hash_tree::good){
			LOGGER; ++fail;
			return;
		}
	}
}

void child_block()
{
	//this test only works with a specific block size
	assert(protocol_tcp::hash_block_size == 512);

	database::init::drop_all();
	database::init::create_all();

	//for this test we only care about file size
	std::string dummy_hash = ""; //causes database space to not be allocated
	std::string dummy_path = "share/bla";
	std::time_t dummy_last_write_time = 123;

	//1 row tree
	{//BEGIN test
	file_info FI(
		dummy_hash,
		dummy_path,
		1 * protocol_tcp::file_block_size,
		dummy_last_write_time
	);
	hash_tree HT(FI);
	std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool> pair;

	//block 0
	pair = HT.tree_block_children(0);
	assert(!pair.second);
	pair = HT.file_block_children(0);
	assert(pair.second);
	assert(pair.first.first == 0); assert(pair.first.second == 1);
	}//END test

	std::cout << "\n";

	//2 row tree
	{//BEGIN test
	file_info FI(
		dummy_hash,
		dummy_path,
		2 * protocol_tcp::file_block_size,
		dummy_last_write_time
	);
	hash_tree HT(FI);
	std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool> pair;

	//block 0
	pair = HT.tree_block_children(0);
	assert(pair.second);
	assert(pair.first.first == 1); assert(pair.first.second == 2);
	pair = HT.file_block_children(0);
	assert(!pair.second);

	//block 1
	pair = HT.tree_block_children(1);
	assert(!pair.second);
	pair = HT.file_block_children(1);
	assert(pair.second);
	assert(pair.first.first == 0); assert(pair.first.second == 2);
	}//END test

	//3 row tree
	{//BEGIN test
	file_info FI(
		dummy_hash,
		dummy_path,
		513 * protocol_tcp::file_block_size,
		dummy_last_write_time
	);
	hash_tree HT(FI);
	std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool> pair;

	//block 0 (row 0)
	pair = HT.tree_block_children(0);
	assert(pair.second);
	assert(pair.first.first == 1); assert(pair.first.second == 2);
	pair = HT.file_block_children(0);
	assert(!pair.second);

	//block 1 (row 1)
	pair = HT.tree_block_children(1);
	assert(pair.second);
	pair = HT.file_block_children(1);
	assert(!pair.second);

	//block 2 (row 2)
	pair = HT.tree_block_children(2);
	assert(!pair.second);
	pair = HT.file_block_children(2);
	assert(pair.second);
	assert(pair.first.first == 0); assert(pair.first.second == 512);
	}//END test
}

int main()
{
	//setup database and make sure hash table clear
	path::override_database_name("hash_tree.db");
	path::override_program_directory("");
	path::create_required_directories();

	create_reassemble(2);
	create_reassemble(3);
	create_reassemble(4);
	create_reassemble(256);
	create_reassemble(257);
	create_reassemble(1024);

	child_block();

	//remove temporary files
	path::remove_temporary_hash_tree_files();
	return fail;
}
