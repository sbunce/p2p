#include "transfer.hpp"

transfer::transfer(const file_info & FI):
	File(FI),
	Hash_Tree(FI)
{

}

void transfer::check()
{
LOGGER << "YIPPIE!";
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
	return Hash_Tree.write_block(block_num, block);
}
