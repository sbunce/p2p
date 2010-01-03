#include "transfer.hpp"

transfer::transfer(const file_info & FI):
	Hash_Tree(FI),
	File(FI),
	Hash_Tree_Block(Hash_Tree.tree_block_count),
	File_Block(Hash_Tree.file_block_count)
{
	assert(FI.file_size != 0);

//DEBUG, need to know if we have all blocks, if we do set block_request members
//appropriately
	boost::shared_ptr<database::table::hash::info>
		hash_info = database::table::hash::find(FI.hash);
	if(hash_info){
		if(hash_info->tree_state == database::table::hash::complete){
			Hash_Tree_Block.add_block_local_all();
		}
	}else{
		throw std::runtime_error("error finding hash tree info");
	}

	boost::shared_ptr<database::table::share::info>
		share_info = database::table::share::find(FI.hash);
	if(share_info){
		if(share_info->file_state == database::table::share::complete){
			File_Block.add_block_local_all();
		}
	}else{
		throw std::runtime_error("error finding share info");
	}

LOGGER << "tree block count " << Hash_Tree.tree_block_count;
}

void transfer::check()
{

}

bool transfer::complete()
{
//DEBUG, finish this function
	return false;
}

boost::uint64_t transfer::file_block_count()
{
	return Hash_Tree.file_block_count;
}

boost::uint64_t transfer::file_size()
{
	return Hash_Tree.file_size;
}

const std::string & transfer::hash()
{
	return Hash_Tree.hash;
}

bool transfer::get_status(unsigned char & byte)
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

bool transfer::next_request_tree(const int connection_ID,
	boost::uint64_t & block_num, unsigned & block_size)
{
LOGGER;
	if(Hash_Tree_Block.next_request(connection_ID, block_num)){
LOGGER << block_num;
		block_size = Hash_Tree.block_size(block_num);
		return true;
	}
	return false;
}

bool transfer::next_request_file(const int connection_ID,
	boost::uint64_t & block_num, unsigned & block_size)
{
	if(File_Block.next_request(connection_ID, block_num)){
		block_size = File.block_size(block_num);
		return true;
	}
	return false;
}

transfer::status transfer::read_file_block(boost::shared_ptr<message::base> & M,
		const boost::uint64_t block_num)
{
/*
	if(File_Block.have_block(block_num)){
		network::buffer block;
		hash_tree::status status = File.read_block(block_num, block);
		if(status == hash_tree::good){
//DEBUG, big copy constructing this message.
			M = boost::shared_ptr<message::base>(new message::block(block));
			return good;
		}else{
			return bad;
		}
	}else{
		return protocol_violated;
	}
*/
}

transfer::status transfer::read_tree_block(boost::shared_ptr<message::base> & M,
	const boost::uint64_t block_num)
{
	if(Hash_Tree_Block.have_block(block_num)){
		network::buffer block;
		hash_tree::status status = Hash_Tree.read_block(block_num, block);
		if(status == hash_tree::good){
//DEBUG, big copy constructing this message.
			M = boost::shared_ptr<message::base>(new message::block(block));
			return good;
		}else{
			return bad;
		}
	}else{
		return protocol_violated;
	}
}

void transfer::register_outgoing_0(const int connection_ID)
{
	Hash_Tree_Block.add_host_complete(connection_ID);
	File_Block.add_host_complete(connection_ID);
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

boost::uint64_t transfer::tree_block_count()
{
	return Hash_Tree.tree_block_count;
}

hash_tree::status transfer::write_tree_block(const boost::uint64_t block_num,
	const network::buffer & block)
{
LOGGER << block_num;
	hash_tree::status status = Hash_Tree.write_block(block_num, block);
	if(status == hash_tree::good){
		Hash_Tree_Block.add_block_local(block_num);
		std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool>
			pair = Hash_Tree.tree_block_children(block_num);
		if(pair.second){
			//approve child hash tree blocks
LOGGER << pair.first.first << " " << pair.first.second;
			for(boost::uint64_t x=pair.first.first; x<pair.first.second; ++x){
				Hash_Tree_Block.approve_block(x);
			}
		}
		pair = Hash_Tree.file_block_children(block_num);
		if(pair.second){
			//approve file blocks that are children of bottom tree row
LOGGER << pair.first.first << " " << pair.first.second;
			for(boost::uint64_t x=pair.first.first; x<pair.first.second; ++x){
				File_Block.approve_block(x);
			}
		}
	}
	return status;
}
