#include "transfer.hpp"

transfer::transfer(const file_info & FI):
	Hash_Tree(FI),
	File(FI),
	Hash_Tree_Block(Hash_Tree.tree_block_count),
	File_Block(Hash_Tree.file_block_count),
	bytes_received(0)
{
	assert(FI.file_size != 0);

	//see if tree complete
	boost::shared_ptr<database::table::hash::info>
		hash_info = database::table::hash::find(FI.hash);
	if(hash_info){
		if(hash_info->tree_state == database::table::hash::complete){
			Hash_Tree_Block.add_block_local_all();
			bytes_received += Hash_Tree.tree_size;
		}
	}else{
		throw std::runtime_error("error finding hash tree info");
	}

	//see if file complete
	boost::shared_ptr<database::table::share::info>
		share_info = database::table::share::find(FI.hash);
	if(share_info){
		if(share_info->file_state == database::table::share::complete){
			File_Block.add_block_local_all();
			bytes_received += Hash_Tree.file_size;
		}
	}else{
		throw std::runtime_error("error finding share info");
	}
}

void transfer::check()
{

}

bool transfer::complete()
{
	return Hash_Tree_Block.complete() && File_Block.complete();
}

unsigned transfer::download_speed()
{
	return Download_Speed.speed();
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
	if(Hash_Tree_Block.next_request(connection_ID, block_num)){
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

unsigned transfer::percent_complete()
{
	return ((double)bytes_received / (Hash_Tree.tree_size + Hash_Tree.file_size)) * 100;
}

transfer::status transfer::read_file_block(boost::shared_ptr<message::base> & M,
		const boost::uint64_t block_num)
{
	if(File_Block.have_block(block_num)){
		network::buffer buf;
		if(File.read_block(block_num, buf)){
			/*
			We can't trust that the local user hasn't modified the file. We hash
			check every block before we send it to detect modified blocks.
			*/
			hash_tree::status status = Hash_Tree.check_file_block(block_num, buf);
			if(status != hash_tree::good){
				return bad;
			}
			Upload_Speed.add(buf.size());
			M = boost::shared_ptr<message::base>(new message::block(buf));
			return good;
		}else{
			return bad;
		}
	}else{
		return protocol_violated;
	}
}

transfer::status transfer::read_tree_block(boost::shared_ptr<message::base> & M,
	const boost::uint64_t block_num)
{
	if(Hash_Tree_Block.have_block(block_num)){
		network::buffer buf;
		hash_tree::status status = Hash_Tree.read_block(block_num, buf);
		if(status == hash_tree::good){
			Upload_Speed.add(buf.size());
			M = boost::shared_ptr<message::base>(new message::block(buf));
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

void transfer::touch()
{
	Download_Speed.add(0);
	Upload_Speed.add(0);
}

boost::uint64_t transfer::tree_block_count()
{
	return Hash_Tree.tree_block_count;
}

unsigned transfer::upload_speed()
{
	return Upload_Speed.speed();
}

transfer::status transfer::write_file_block(const int connection_ID,
	const boost::uint64_t block_num, const network::buffer & buf)
{
	if(File_Block.have_block(block_num)){
		//don't write block which already exists
		return good;
	}
	Download_Speed.add(buf.size());
	hash_tree::status status = Hash_Tree.check_file_block(block_num, buf);
	if(status == hash_tree::good){
		if(File.write_block(block_num, buf)){
			File_Block.add_block_local(connection_ID, block_num);
			bytes_received += buf.size();
			return good;
		}else{
			//failed to write block
			return bad;
		}
	}else if(status == hash_tree::io_error){
		return bad;
	}else{
		return protocol_violated;
	}
}

transfer::status transfer::write_tree_block(const int connection_ID,
	const boost::uint64_t block_num, const network::buffer & buf)
{
	if(Hash_Tree_Block.have_block(block_num)){
		//don't write block which already exists
		return good;
	}
	Download_Speed.add(buf.size());
	hash_tree::status status = Hash_Tree.write_block(block_num, buf);
	if(status == hash_tree::good){
		Hash_Tree_Block.add_block_local(connection_ID, block_num);
		bytes_received += buf.size();
		std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool>
			pair = Hash_Tree.tree_block_children(block_num);
		if(pair.second){
			//approve child hash tree blocks
			for(boost::uint64_t x=pair.first.first; x<pair.first.second; ++x){
				Hash_Tree_Block.approve_block(x);
			}
		}
		pair = Hash_Tree.file_block_children(block_num);
		if(pair.second){
			//approve file blocks that are children of bottom tree row
			for(boost::uint64_t x=pair.first.first; x<pair.first.second; ++x){
				File_Block.approve_block(x);
			}
		}
		return good;
	}else if(status == hash_tree::bad){
		return protocol_violated;
	}else if(status == hash_tree::io_error){
		return bad;
	}
}
