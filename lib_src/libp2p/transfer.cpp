#include "transfer.hpp"

//BEGIN next_request
transfer::next_request::next_request(
	const boost::uint64_t block_num_in,
	const unsigned block_size_in
):
	block_num(block_num_in),
	block_size(block_size_in)
{

}

transfer::next_request::next_request(const next_request & NR):
	block_num(NR.block_num),
	block_size(NR.block_size)
{

}
//END next_request

transfer::transfer(const file_info & FI):
	Hash_Tree(FI),
	File(FI),
	Tree_Block(Hash_Tree.tree_block_count),
	File_Block(Hash_Tree.file_block_count),
	bytes_received(0),
	Download_Speed(new net::speed_calc()),
	Upload_Speed(new net::speed_calc())
{
	assert(FI.file_size != 0);

	//see if tree complete
	boost::shared_ptr<db::table::hash::info>
		hash_info = db::table::hash::find(FI.hash);
	if(hash_info){
		if(hash_info->tree_state == db::table::hash::complete){
			Tree_Block.add_block_local_all();
			bytes_received += Hash_Tree.tree_size;
		}
	}else{
		throw std::runtime_error("error finding hash tree info");
	}

	//see if file complete
	boost::shared_ptr<db::table::share::info>
		share_info = db::table::share::find(FI.hash);
	if(share_info){
		if(share_info->file_state == db::table::share::complete){
			File_Block.add_block_local_all();
			bytes_received += Hash_Tree.file_size;
		}
	}else{
		throw std::runtime_error("error finding share info");
	}
}

void transfer::check()
{
	net::buffer buf;
	buf.reserve(protocol_tcp::file_block_size);

	//only check tree blocks with good parents
	Tree_Block.approve_block(0);
	for(boost::uint64_t block_num=0; block_num<Hash_Tree.tree_block_count; ++block_num){
		boost::this_thread::interruption_point();
		if(!Tree_Block.is_approved(block_num)){
			continue;
		}
		buf.clear();
		hash_tree::status status = Hash_Tree.read_block(block_num, buf);
		if(status == hash_tree::io_error){
			LOG << "stub: handle io_error when hash checking";
			exit(1);
		}
		status = Hash_Tree.check_tree_block(block_num, buf);
		if(status == hash_tree::good){
			bytes_received += buf.size();
			Tree_Block.add_block_local(block_num);
			std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool>
				pair = Hash_Tree.tree_block_children(block_num);
			if(pair.second){
				//approve child hash tree blocks
				for(boost::uint64_t x=pair.first.first; x<pair.first.second; ++x){
					Tree_Block.approve_block(x);
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
			LOG << "stub: handle io_error when hash checking";
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
			LOG << "stub: handle io_error when hash checking";
			exit(1);
		}
	}
}

bool transfer::complete()
{
	return Tree_Block.complete() && File_Block.complete();
}

unsigned transfer::download_count()
{
	return Tree_Block.download_count();
}

unsigned transfer::download_speed()
{
	Download_Speed->add(0);
	return Download_Speed->speed();
}

const boost::shared_ptr<net::speed_calc> & transfer::download_speed_calc()
{
	return Download_Speed;
}

void transfer::download_subscribe(const int connection_ID, const net::endpoint & ep,
	const bit_field & tree_BF, const bit_field & file_BF)
{
	Peer.add(connection_ID, ep);
	Tree_Block.download_subscribe(connection_ID, tree_BF);
	File_Block.download_subscribe(connection_ID, file_BF);
}

void transfer::download_unsubscribe(const int connection_ID)
{
	Tree_Block.download_unsubscribe(connection_ID);
	File_Block.download_unsubscribe(connection_ID);
}

boost::uint64_t transfer::file_block_count()
{
	return Hash_Tree.file_block_count;
}

unsigned transfer::file_percent_complete()
{
	return File_Block.percent_complete();
}

boost::uint64_t transfer::file_size()
{
	return Hash_Tree.file_size;
}

std::list<p2p::transfer_info::host_element> transfer::host()
{
	std::list<p2p::transfer_info::host_element> tmp;
	p2p::transfer_info::host_element test;
	test.IP = "123.123.123.123";
	test.port = "1234";
	test.download_speed = 1024;
	test.upload_speed = 1024;
	tmp.push_back(test);
	return tmp;
}

unsigned transfer::upload_count()
{
	return Tree_Block.upload_count();
}

boost::optional<boost::uint64_t> transfer::next_have_file(const int connection_ID)
{
	return File_Block.next_have(connection_ID);
}

boost::optional<boost::uint64_t> transfer::next_have_tree(const int connection_ID)
{
	return Tree_Block.next_have(connection_ID);
}

boost::optional<net::endpoint> transfer::next_peer(const int connection_ID)
{
	return Peer.get(connection_ID);
}

boost::optional<transfer::next_request> transfer::next_request_tree(const int connection_ID)
{
	if(boost::optional<boost::uint64_t> block_num = Tree_Block.next_request(connection_ID)){
		unsigned block_size = Hash_Tree.block_size(*block_num);
		return next_request(*block_num, block_size);
	}
	return boost::optional<next_request>();
}

boost::optional<transfer::next_request> transfer::next_request_file(const int connection_ID)
{
	if(boost::optional<boost::uint64_t> block_num = File_Block.next_request(connection_ID)){
		unsigned block_size = File.block_size(*block_num);
		return next_request(*block_num, block_size);
	}
	return boost::optional<next_request>();
}

unsigned transfer::percent_complete()
{
	return ((double)bytes_received / (Hash_Tree.tree_size + Hash_Tree.file_size)) * 100;
}

std::pair<boost::shared_ptr<message_tcp::send::base>, transfer::status>
	transfer::read_file_block(const boost::uint64_t block_num)
{
	if(File_Block.have_block(block_num)){
		net::buffer buf;
		if(File.read_block(block_num, buf)){
			/*
			We can't trust that the local user hasn't modified the file. We hash
			check every block before we send it to detect modified blocks.
			*/
			hash_tree::status status = Hash_Tree.check_file_block(block_num, buf);
			if(status == hash_tree::good){
				return std::make_pair(boost::shared_ptr<message_tcp::send::base>(
					new message_tcp::send::block(buf, Upload_Speed)), good);
			}else{
				return std::make_pair(boost::shared_ptr<message_tcp::send::base>(), bad);
			}
		}else{
			return std::make_pair(boost::shared_ptr<message_tcp::send::base>(), bad);
		}
	}else{
		return std::make_pair(boost::shared_ptr<message_tcp::send::base>(), protocol_violated);
	}
}

std::pair<boost::shared_ptr<message_tcp::send::base>, transfer::status>
	transfer::read_tree_block(const boost::uint64_t block_num)
{
	if(Tree_Block.have_block(block_num)){
		net::buffer buf;
		hash_tree::status status = Hash_Tree.read_block(block_num, buf);
		if(status == hash_tree::good){
			return std::make_pair(boost::shared_ptr<message_tcp::send::base>(
				new message_tcp::send::block(buf, Upload_Speed)), good);
		}else{
			return std::make_pair(boost::shared_ptr<message_tcp::send::base>(), bad);
		}
	}else{
		return std::make_pair(boost::shared_ptr<message_tcp::send::base>(), protocol_violated);
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
	Tree_Block.add_block_remote(connection_ID, block_num);
}

boost::optional<std::string> transfer::root_hash()
{
	char buf[8 + SHA1::bin_size];
	std::memcpy(buf, convert::int_to_bin(file_size()).data(), 8);
	if(!db::pool::get()->blob_read(Hash_Tree.blob, buf+8, SHA1::bin_size, 0)){
		return boost::optional<std::string>();
	}
	SHA1 SHA(buf, 8 + SHA1::bin_size);
	if(SHA.hex() != Hash_Tree.hash){
		return boost::optional<std::string>();
	}
	return convert::bin_to_hex(std::string(buf+8, SHA1::bin_size));
}

boost::uint64_t transfer::tree_block_count()
{
	return Hash_Tree.tree_block_count;
}

unsigned transfer::tree_percent_complete()
{
	return Tree_Block.percent_complete();
}


boost::uint64_t transfer::tree_size()
{
	return Hash_Tree.tree_size;
}

unsigned transfer::upload_speed()
{
	Upload_Speed->add(0);
	return Upload_Speed->speed();
}

transfer::local_BF transfer::upload_subscribe(const int connection_ID,
	const net::endpoint & ep, const boost::function<void()> trigger_tick)
{
	Peer.add(connection_ID, ep);
	local_BF tmp;
	tmp.tree_BF = Tree_Block.upload_subscribe(connection_ID, trigger_tick);
	tmp.file_BF = File_Block.upload_subscribe(connection_ID, trigger_tick);
	return tmp;
}

void transfer::upload_unsubscribe(const int connection_ID)
{
	Tree_Block.upload_unsubscribe(connection_ID);
	File_Block.upload_unsubscribe(connection_ID);
}

transfer::status transfer::write_file_block(const int connection_ID,
	const boost::uint64_t block_num, const net::buffer & buf)
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
				db::table::share::set_state(Hash_Tree.hash, db::table::share::complete);
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
	const boost::uint64_t block_num, const net::buffer & buf)
{
	if(Tree_Block.have_block(block_num)){
		/*
		Don't write block which already exists.
		Note: Multiple threads might make it past here with the same block but
			that is ok.
		*/
		return good;
	}
	hash_tree::status status = Hash_Tree.write_block(block_num, buf);
	if(status == hash_tree::good){
		Tree_Block.add_block_local(connection_ID, block_num);
		bytes_received += buf.size();
		std::pair<std::pair<boost::uint64_t, boost::uint64_t>, bool>
			pair = Hash_Tree.tree_block_children(block_num);
		if(pair.second){
			//approve child hash tree blocks
			for(boost::uint64_t x=pair.first.first; x<pair.first.second; ++x){
				Tree_Block.approve_block(x);
			}
		}
		pair = Hash_Tree.file_block_children(block_num);
		if(pair.second){
			//approve file blocks that are children of bottom tree row
			for(boost::uint64_t x=pair.first.first; x<pair.first.second; ++x){
				File_Block.approve_block(x);
			}
		}
		if(Tree_Block.complete()){
			db::table::hash::set_state(Hash_Tree.hash, db::table::hash::complete);
		}
		return good;
	}else if(status == hash_tree::bad){
		return protocol_violated;
	}else if(status == hash_tree::io_error){
		return bad;
	}
}
