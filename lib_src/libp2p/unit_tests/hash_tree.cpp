//custom
#include "../db_all.hpp"
#include "../hash_tree.hpp"
#include "../path.hpp"
#include "../protocol_tcp.hpp"
#include "../share.hpp"

//include
#include <logger.hpp>
#include <unit_test.hpp>

//standard
#include <algorithm>

int fail(0);

//create test files to hash if they don't already exist
void create_file(const file_info & FI)
{
	std::fstream fout(FI.path.c_str(), std::ios::out | std::ios::binary
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
	db::init::drop_all();
	db::init::create_all();

	//setup test file
	file_info FI;
	std::stringstream ss;
	ss << path::share_dir() << size << "_block";
	FI.path = ss.str();
	FI.file_size = size * protocol_tcp::file_block_size;
	create_file(FI);

	//create hash tree and check it
	if(hash_tree::create(FI) != hash_tree::good){
		LOG; ++fail;
		return;
	}

	//hash tree should be good
	hash_tree HT(FI);
	if(HT.check() != hash_tree::good){
		LOG; ++fail;
		return;
	}

	//read all blocks from the hash tree in to memory
	std::vector<net::buffer> block; //location in vector is block number
	for(boost::uint32_t x=0; x<HT.TI.tree_block_count; ++x){
		net::buffer buf;
		HT.read_block(x, buf);
		block.push_back(buf);
	}

	db::init::drop_all();
	db::init::create_all();

	//reassemble
	hash_tree HT_reassemble(FI);
	for(boost::uint64_t x=0; x<HT.TI.tree_block_count; ++x){
		HT_reassemble.write_block(x, block[x]);
	}

	//hash tree should be good
	if(HT_reassemble.check() != hash_tree::good){
		LOG; ++fail;
		return;
	}

	//check file blocks with rebuilt tree
	net::buffer buf;
	std::fstream fin(FI.path.c_str(), std::ios::in | std::ios::binary);
	if(!fin.good()){
		LOG; ++fail;
		return;
	}
	for(boost::uint64_t x=0; x<HT_reassemble.TI.file_block_count; ++x){
		buf.reserve(protocol_tcp::file_block_size);
		fin.read(reinterpret_cast<char *>(buf.data()), protocol_tcp::file_block_size);
		if(!fin.good()){
			LOG; ++fail;
			return;
		}
		buf.resize(fin.gcount());
		if(HT_reassemble.check_file_block(x, buf) != hash_tree::good){
			LOG; ++fail;
			return;
		}
	}
}

int main()
{
	unit_test::timeout(60);

	//setup database and make sure hash table clear
	path::set_db_file_name("hash_tree.db");
	path::set_program_dir("");
	path::create_dirs();

	create_reassemble(2);
	create_reassemble(3);
	create_reassemble(4);
	create_reassemble(256);
	create_reassemble(257);
	create_reassemble(1024);

	//remove temporary files
	path::remove_tmp_tree_files();
	return fail;
}
