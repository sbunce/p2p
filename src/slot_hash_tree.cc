#include "slot_hash_tree.h"

slot_hash_tree::slot_hash_tree(
	std::string * IP_in,
	const std::string & root_hash_in,
	const boost::uint64_t & file_size_in
):
	Tree_Info(root_hash_in, file_size_in),
	percent(0),
	highest_block_seen(0)
{
	IP = IP_in;
	root_hash = root_hash_in;
	file_size = file_size_in;
}

void slot_hash_tree::info(std::vector<upload_info> & UI)
{
	UI.push_back(upload_info(
		root_hash,
		*IP,
		Tree_Info.get_tree_size(),
		global::HASH_DIRECTORY + slot::root_hash,
		Speed_Calculator.speed(),
		percent
	));
}

void slot_hash_tree::send_block(const std::string & request, std::string & send)
{
	boost::uint64_t block_number = convert::decode<boost::uint64_t>(request.substr(2, 8));
	client_server_bridge::download_mode DM = client_server_bridge::is_downloading(root_hash);
	if(DM == client_server_bridge::DOWNLOAD_HASH_TREE){
		//client requested a block from a hash tree the client is currently downloading
		send += global::P_WAIT;
		Speed_Calculator.update(global::P_WAIT_SIZE);
		std::cout << "HASH TREE UPLOAD WHILE DOWNLOAD NOT IMPLEMENTED YET\n";
	}else{
		//hash tree not downloading, see if it is in the share
		if(Hash_Tree.read_block(Tree_Info, block_number, send)){
			send = global::P_BLOCK + send;
			Speed_Calculator.update(send.size());
			update_percent(block_number);
		}else{
			//invalid block requested
			logger::debug(LOGGER_P1,*IP," requested invalid block");
			DB_blacklist::add(*IP);
		}
	}
}

void slot_hash_tree::update_percent(const boost::uint64_t & latest_block)
{
	if(latest_block != 0 && latest_block > highest_block_seen){
		percent = (int)(((double)latest_block / (double)Tree_Info.get_block_count())*100);
		highest_block_seen = latest_block;
		assert(percent <= 100);
	}
}
