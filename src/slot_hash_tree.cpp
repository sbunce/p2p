#include "slot_hash_tree.hpp"

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

	#ifdef CORRUPT_HASH_BLOCKS
	std::srand(time(NULL));
	#endif
}

void slot_hash_tree::info(std::vector<upload_info> & UI)
{
	UI.push_back(upload_info(
		root_hash,
		*IP,
		Tree_Info.get_tree_size(),
		"",
		Speed_Calculator.speed(),
		percent
	));
}

void slot_hash_tree::send_block(const std::string & request, std::string & send)
{
	boost::uint64_t block_num = convert::decode<boost::uint64_t>(request.substr(2, 8));
	if(block_num >= Tree_Info.get_block_count()){
		//client requested block higher than last block
		LOGGER << *IP << " requested block higher than maximum, blacklist";
		DB_Blacklist.add(*IP);
		return;
	}

	block_arbiter::download_state DS;
	DS = block_arbiter::hash_block_available(root_hash, block_num);
	if(DS == block_arbiter::DOWNLOADING_NOT_AVAILABLE){
		//hash tree downloading but it client doesn't yet have requested block
		LOGGER << "sending P_WAIT to " << *IP;
		send += global::P_WAIT;
		Speed_Calculator.update(global::P_WAIT_SIZE);
	}else{
		//hash block available
		if(Hash_Tree.read_block(Tree_Info, block_num, send)){
			#ifdef CORRUPT_HASH_BLOCK_TEST
			if(std::rand() % 5 == 0){
				LOGGER << "CORRUPT HASH BLOCK TEST, block " << block_num << " -> " << *IP;
				send[0] = ~send[0];
			}
			#endif
			send = global::P_BLOCK + send;
			Speed_Calculator.update(send.size());
			update_percent(block_num);
		}else{
			//hash block no longer available, or the client was blacklisted for sending invalid request
			send += global::P_ERROR;
			Speed_Calculator.update(global::P_ERROR_SIZE);
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
