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
	Tree_Block(Hash_Tree.TI.tree_block_count),
	File_Block(Hash_Tree.TI.file_block_count),
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
			bytes_received += Hash_Tree.TI.tree_size;
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
			bytes_received += Hash_Tree.TI.file_size;
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
	for(boost::uint64_t block_num=0; block_num<Hash_Tree.TI.tree_block_count; ++block_num){
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
				pair = Hash_Tree.TI.tree_block_children(block_num);
			if(pair.second){
				//approve child hash tree blocks
				for(boost::uint64_t x=pair.first.first; x<pair.first.second; ++x){
					Tree_Block.approve_block(x);
				}
			}
			pair = Hash_Tree.TI.file_block_children(block_num);
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
	for(boost::uint64_t block_num=0; block_num<Hash_Tree.TI.file_block_count; ++block_num){
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

unsigned transfer::download_hosts()
{
	return Tree_Block.download_hosts();
}

unsigned transfer::download_speed()
{
	Download_Speed->add(0);
	return Download_Speed->speed();
}

speed_composite transfer::download_speed_composite(const int connection_ID)
{
	speed_composite SC;
	SC.add_calc(Download_Speed);
	SC.add_calc(Peer.download_speed_calc(connection_ID));
	return SC;
}

void transfer::download_reg(const int connection_ID, const net::endpoint & ep,
	const bit_field & tree_BF, const bit_field & file_BF)
{
	Peer.reg(connection_ID, ep);
	Tree_Block.download_reg(connection_ID, tree_BF);
	File_Block.download_reg(connection_ID, file_BF);
}

void transfer::download_unreg(const int connection_ID)
{
	Peer.unreg(connection_ID);
	Tree_Block.download_unreg(connection_ID);
	File_Block.download_unreg(connection_ID);
}

boost::uint64_t transfer::file_block_count()
{
	return Hash_Tree.TI.file_block_count;
}

unsigned transfer::file_percent_complete()
{
	return File_Block.percent_complete();
}

boost::uint64_t transfer::file_size()
{
	return Hash_Tree.TI.file_size;
}

std::list<p2p::transfer_info::host_element> transfer::host_info()
{
	return Peer.host_info();
}

unsigned transfer::upload_hosts()
{
	return Tree_Block.upload_hosts();
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
		unsigned block_size = Hash_Tree.TI.block_size(*block_num);
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
	return ((double)bytes_received / (Hash_Tree.TI.tree_size + Hash_Tree.TI.file_size)) * 100;
}

std::pair<net::buffer, transfer::status> transfer::read_file_block(
	const boost::uint64_t block_num)
{
	std::pair<net::buffer, status> p;
	if(File_Block.have_block(block_num)){
		if(File.read_block(block_num, p.first)){
			/*
			We can't trust that the local user hasn't modified the file. We hash
			check every block before we send it to detect modified blocks.
			*/
			hash_tree::status status = Hash_Tree.check_file_block(block_num, p.first);
			if(status == hash_tree::good){
				p.second = good;
				return p;
			}else{
				p.second = bad;
				return p;
			}
		}else{
			p.second = bad;
			return p;
		}
	}else{
		p.second = protocol_violated;
		return p;
	}
}

std::pair<net::buffer, transfer::status> transfer::read_tree_block(
	const boost::uint64_t block_num)
{
	std::pair<net::buffer, status> p;
	if(Tree_Block.have_block(block_num)){
		hash_tree::status status = Hash_Tree.read_block(block_num, p.first);
		if(status == hash_tree::good){
			p.second = good;
			return p;
		}else{
			p.second = bad;
			return p;
		}
	}else{
		p.second = protocol_violated;
		return p;
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
	return Hash_Tree.root_hash();
}

boost::uint64_t transfer::tree_block_count()
{
	return Hash_Tree.TI.tree_block_count;
}

unsigned transfer::tree_percent_complete()
{
	return Tree_Block.percent_complete();
}

boost::uint64_t transfer::tree_size()
{
	return Hash_Tree.TI.tree_size;
}

unsigned transfer::upload_speed()
{
	Upload_Speed->add(0);
	return Upload_Speed->speed();
}

speed_composite transfer::upload_speed_composite(const int connection_ID)
{
	speed_composite SC;
	SC.add_calc(Upload_Speed);
	SC.add_calc(Peer.upload_speed_calc(connection_ID));
	return SC;
}

transfer::local_BF transfer::upload_reg(const int connection_ID,
	const net::endpoint & ep, const boost::function<void()> trigger_tick)
{
	Peer.reg(connection_ID, ep);
	local_BF tmp;
	tmp.tree_BF = Tree_Block.upload_reg(connection_ID, trigger_tick);
	tmp.file_BF = File_Block.upload_reg(connection_ID, trigger_tick);
	return tmp;
}

void transfer::upload_unreg(const int connection_ID)
{
	Peer.unreg(connection_ID);
	Tree_Block.upload_unreg(connection_ID);
	File_Block.upload_unreg(connection_ID);
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
				db::table::share::set_state(Hash_Tree.TI.hash, db::table::share::complete);
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
			pair = Hash_Tree.TI.tree_block_children(block_num);
		if(pair.second){
			//approve child hash tree blocks
			for(boost::uint64_t x=pair.first.first; x<pair.first.second; ++x){
				Tree_Block.approve_block(x);
			}
		}
		pair = Hash_Tree.TI.file_block_children(block_num);
		if(pair.second){
			//approve file blocks that are children of bottom tree row
			for(boost::uint64_t x=pair.first.first; x<pair.first.second; ++x){
				File_Block.approve_block(x);
			}
		}
		if(Tree_Block.complete()){
			db::table::hash::set_state(Hash_Tree.TI.hash, db::table::hash::complete);
		}
		return good;
	}else if(status == hash_tree::bad){
		return protocol_violated;
	}else if(status == hash_tree::io_error){
		return bad;
	}
}
