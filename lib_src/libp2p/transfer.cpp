#include "transfer.hpp"

transfer::transfer(const file_info & FI):
	Hash_Tree(FI),
	File(FI),
	Hash_Tree_Block(Hash_Tree.tree_block_count),
	File_Block(Hash_Tree.file_block_count)
{
	assert(FI.file_size != 0);
}

void transfer::check()
{

}

bool transfer::complete()
{
//DEBUG, finish this function
	return false;
}

boost::uint64_t transfer::file_size()
{
	return Hash_Tree.file_size;
}

const std::string & transfer::hash()
{
	return Hash_Tree.hash;
}

void transfer::register_outgoing_0(const int connection_ID)
{
	Hash_Tree_Block.add_host_complete(connection_ID);
	File_Block.add_host_complete(connection_ID);
}

bool transfer::request_hash_tree_block(const int connection_ID,
	boost::uint64_t & block_num, unsigned & block_size,
	boost::uint64_t & tree_block_count)
{
	if(Hash_Tree_Block.next_request(connection_ID, block_num)){
		block_size = Hash_Tree.block_size(block_num);
		tree_block_count = Hash_Tree.tree_block_count;
		return true;
	}
	return false;
}

bool transfer::request_file_block(const int connection_ID,
	boost::uint64_t & block_num, unsigned & block_size,
	boost::uint64_t & file_block_count)
{
	if(File_Block.next_request(connection_ID, block_num)){
		block_size = File.block_size(block_num);
		file_block_count = Hash_Tree.file_block_count;
		return true;
	}
	return false;
}

bool transfer::root_hash(std::string & RH)
{
	char buf[8 + SHA1::bin_size];
	std::memcpy(buf, convert::encode(file_size()).data(), 8);
	if(!database::pool::get()->blob_read(Hash_Tree.blob, buf+8, SHA1::bin_size, 0)){
		return false;
	}
	SHA1 SHA;
	SHA.init();
	SHA.load(buf, 8 + SHA1::bin_size);
	SHA.end();
	if(SHA.hex() != hash()){
		return false;
	}
	RH = convert::bin_to_hex(buf+8, SHA1::bin_size);
	return true;
}

bool transfer::status(unsigned char & byte)
{
	byte = 0;
	return true;
	/*
	if(slot_iter->Hash_Tree.tree_complete() && slot_iter->Hash_Tree.file_complete()){
		status = 0;
	}else if(slot_iter->Hash_Tree.tree_complete() && !slot_iter->Hash_Tree.file_complete()){
		status = 1;
LOGGER << "stub: add support for incomplete"; exit(1);
	}else if(!slot_iter->Hash_Tree.tree_complete() && slot_iter->Hash_Tree.file_complete()){
		status = 2;
LOGGER << "stub: add support for incomplete"; exit(1);
	}else if(!slot_iter->Hash_Tree.tree_complete() && !slot_iter->Hash_Tree.file_complete()){
		status = 3;
LOGGER << "stub: add support for incomplete"; exit(1);
	}
*/
}

hash_tree::status transfer::write_hash_tree_block(const boost::uint64_t block_num,
	const network::buffer & block)
{
//DEBUG, need to record block in Hash_Tree_Block

	hash_tree::status Status = Hash_Tree.write_block(block_num, block);
}
