#include "transfer.hpp"

transfer::transfer(const file_info & FI):
	Hash_Tree(FI),
	File(FI),
	Hash_Tree_Block(Hash_Tree.tree_block_count),
	File_Block(Hash_Tree.file_block_count),
	bytes_received(0),
	Download_Speed(new network::speed_calculator()),
	Upload_Speed(new network::speed_calculator())
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
	network::buffer buf;
	buf.reserve(protocol::file_block_size);

	//only check tree blocks with good parents
	Hash_Tree_Block.approve_block(0);
	for(boost::uint64_t block_num=0; block_num<Hash_Tree.tree_block_count; ++block_num){
		boost::this_thread::interruption_point();
		if(!Hash_Tree_Block.is_approved(block_num)){
			continue;
		}
		buf.clear();
		hash_tree::status status = Hash_Tree.read_block(block_num, buf);
		if(status == hash_tree::io_error){
			LOGGER << "stub: handle io_error when hash checking";
			exit(1);
		}
		status = Hash_Tree.check_tree_block(block_num, buf);
		if(status == hash_tree::good){
			bytes_received += buf.size();
			Hash_Tree_Block.add_block_local(block_num);
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
		}else if(status == hash_tree::io_error){
			LOGGER << "stub: handle io_error when hash checking";
			exit(1);
		}
	}

	//only check file blocks with good hash tree parents
	for(boost::uint64_t block_num=0; block_num<Hash_Tree.file_block_count; ++block_num){
		boost::this_thread::interruption_point();
		if(!File_Block.is_approved(block_num)){
			continue;
		}
		buf.clear();
		File.read_block(block_num, buf);
		hash_tree::status status = Hash_Tree.check_file_block(block_num, buf);
		if(status == hash_tree::good){
			bytes_received += buf.size();
			File_Block.add_block_local(block_num);
		}else if(status == hash_tree::io_error){
			LOGGER << "stub: handle io_error when hash checking";
			exit(1);
		}
	}
}

bool transfer::complete()
{
	return Hash_Tree_Block.complete() && File_Block.complete();
}

unsigned transfer::download_speed()
{
	return Download_Speed->speed();
}

boost::shared_ptr<network::speed_calculator> transfer::download_speed_calculator()
{
	return Download_Speed;
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

unsigned transfer::incoming_count()
{
	return Hash_Tree_Block.incoming_count();
}

void transfer::incoming_subscribe(const int connection_ID,
	const boost::function<void(const int)> trigger_tick, bit_field & tree_BF,
	bit_field & file_BF)
{
	Hash_Tree_Block.incoming_subscribe(connection_ID, trigger_tick, tree_BF);
	File_Block.incoming_subscribe(connection_ID, trigger_tick, file_BF);
}

void transfer::incoming_unsubscribe(const int connection_ID)
{
	Hash_Tree_Block.incoming_unsubscribe(connection_ID);
	File_Block.incoming_unsubscribe(connection_ID);
}

unsigned transfer::outgoing_count()
{
	return Hash_Tree_Block.outgoing_count();
}

void transfer::outgoing_subscribe(const int connection_ID, bit_field & tree_BF,
	bit_field & file_BF)
{
	Hash_Tree_Block.outgoing_subscribe(connection_ID, tree_BF);
	File_Block.outgoing_subscribe(connection_ID, file_BF);
}

void transfer::outgoing_unsubscribe(const int connection_ID)
{
	Hash_Tree_Block.outgoing_unsubscribe(connection_ID);
	File_Block.outgoing_unsubscribe(connection_ID);
}

bool transfer::next_have_file(const int connection_ID, boost::uint64_t & block_num)
{
	return File_Block.next_have(connection_ID, block_num);
}

bool transfer::next_have_tree(const int connection_ID, boost::uint64_t & block_num)
{
	return Hash_Tree_Block.next_have(connection_ID, block_num);
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
//DEBUG, large copy
			M = boost::shared_ptr<message::base>(new message::block(buf, Upload_Speed));
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
//DEBUG, large copy
			M = boost::shared_ptr<message::base>(new message::block(buf, Upload_Speed));
			return good;
		}else{
			return bad;
		}
	}else{
		return protocol_violated;
	}
}

void transfer::recv_have_file_block(const int connection_ID,
	const boost::uint64_t block_num)
{
	File_Block.add_block_remote(connection_ID, block_num);
}

void transfer::recv_have_hash_tree_block(const int connection_ID,
	const boost::uint64_t block_num)
{
	Hash_Tree_Block.add_block_remote(connection_ID, block_num);
}

bool transfer::root_hash(std::string & RH)
{
	char buf[8 + SHA1::bin_size];
	std::memcpy(buf, convert::encode(file_size()).data(), 8);
	if(!database::pool::get()->blob_read(Hash_Tree.blob, buf+8, SHA1::bin_size, 0)){
		return false;
	}
	SHA1 SHA(buf, 8 + SHA1::bin_size);
	if(SHA.hex() != hash()){
		return false;
	}
	RH = convert::bin_to_hex(buf+8, SHA1::bin_size);
	return true;
}

void transfer::touch()
{
	Download_Speed->add(0);
	Upload_Speed->add(0);
}

boost::uint64_t transfer::tree_block_count()
{
	return Hash_Tree.tree_block_count;
}

unsigned transfer::upload_speed()
{
	return Upload_Speed->speed();
}

transfer::status transfer::write_file_block(const int connection_ID,
	const boost::uint64_t block_num, const network::buffer & buf)
{
	if(File_Block.have_block(block_num)){
		/*
		Don't write block which already exists.
		Note: Multiple threads might make it past here with the same block but
			that is ok.
		*/
		return good;
	}
	hash_tree::status status = Hash_Tree.check_file_block(block_num, buf);
	if(status == hash_tree::good){
		if(File.write_block(block_num, buf)){
			File_Block.add_block_local(connection_ID, block_num);
			bytes_received += buf.size();
			if(File_Block.complete()){
				database::table::share::set_state(Hash_Tree.hash, database::table::share::complete);
			}
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
		/*
		Don't write block which already exists.
		Note: Multiple threads might make it past here with the same block but
			that is ok.
		*/
		return good;
	}
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
		if(Hash_Tree_Block.complete()){
			database::table::hash::set_state(Hash_Tree.hash, database::table::hash::complete);
		}
		return good;
	}else if(status == hash_tree::bad){
		return protocol_violated;
	}else if(status == hash_tree::io_error){
		return bad;
	}
}
