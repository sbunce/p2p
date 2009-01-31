#include "client_server_bridge.hpp"

client_server_bridge * client_server_bridge::Client_Server_Bridge = NULL;
boost::mutex client_server_bridge::Mutex;
atomic_bool client_server_bridge::server_index_blocked(true);

client_server_bridge::client_server_bridge()
{

}

void client_server_bridge::start_hash_tree_priv(const std::string & hash)
{
	//assert file is not started, hash tree must download before file
	assert(File_State.find(hash) == File_State.end());
	Hash_Tree_State.insert(std::make_pair(hash, hash_tree_state()));
}

void client_server_bridge::start_file_priv(const std::string & hash, const boost::uint64_t & file_block_count)
{
	Hash_Tree_State.erase(hash);
	File_State.insert(std::make_pair(hash, file_state(file_block_count)));
}

void client_server_bridge::finish_download_priv(const std::string & hash)
{
	Hash_Tree_State.erase(hash);
	File_State.erase(hash);
}

void client_server_bridge::update_hash_tree_highest_priv(const std::string & hash, const boost::uint64_t & block_num)
{
	std::map<std::string, hash_tree_state>::iterator iter;
	iter = Hash_Tree_State.find(hash);
	assert(iter != Hash_Tree_State.end());
	iter->second.initialized = true;
	iter->second.highest_available = block_num;
}

void client_server_bridge::add_file_block_priv(const std::string & hash, const boost::uint64_t & block_num)
{
	std::map<std::string, file_state>::iterator iter;
	iter = File_State.find(hash);
	assert(iter != File_State.end());
	iter->second.initialized = true;
	iter->second.Contiguous.insert(block_num);
	iter->second.Contiguous.trim_contiguous();
}

bool client_server_bridge::is_downloading_priv(const std::string & hash)
{
	std::map<std::string, hash_tree_state>::iterator h_iter;
	h_iter = Hash_Tree_State.find(hash);
	std::map<std::string, file_state>::iterator f_iter;
	f_iter = File_State.find(hash);
	return h_iter != Hash_Tree_State.end() || f_iter != File_State.end();
}

client_server_bridge::download_state client_server_bridge::hash_block_available_priv(const std::string & hash, const boost::uint64_t & block_number)
{
	std::map<std::string, file_state>::iterator iter;
	iter = File_State.find(hash);
	if(iter == File_State.end()){
		//file not downloading, check if hash downloading
		std::map<std::string, hash_tree_state>::iterator iter;
		iter = Hash_Tree_State.find(hash);
		if(iter == Hash_Tree_State.end()){
			return NOT_DOWNLOADING;
		}else{
			//hash tree downloading, check if block available
			if(iter->second.initialized && block_number <= iter->second.highest_available){
				return DOWNLOADING_AVAILABLE;
			}else{
				//update_hash_tree_highest has never been called, hash tree download just started
				return DOWNLOADING_NOT_AVAILABLE;
			}
		}
	}else{
		//file downloading which implies hash tree complete
		return DOWNLOADING_AVAILABLE;
	}
}

client_server_bridge::download_state client_server_bridge::file_block_available_priv(const std::string & hash, const boost::uint64_t & block_num)
{
	std::map<std::string, file_state>::iterator iter;
	iter = File_State.find(hash);
	if(iter == File_State.end()){
		/*
		File is not downloading, check to see if the hash tree for the file is
		downloading. If it is we will say we are downloading the file but do not
		yet have the block.
		*/
		std::map<std::string, hash_tree_state>::iterator iter;
		iter = Hash_Tree_State.find(hash);
		if(iter == Hash_Tree_State.end()){
			return NOT_DOWNLOADING;
		}else{
			return DOWNLOADING_NOT_AVAILABLE;
		}
	}else{
		//file is downloading, check to see if block available
		if(iter->second.initialized){
			if(iter->second.Contiguous.start_range() == 0){
				//no file blocks available
				return DOWNLOADING_NOT_AVAILABLE;
			}else{
				if(block_num < iter->second.Contiguous.start_range()){
					return DOWNLOADING_AVAILABLE;
				}else{
					//block not in contiguous space, check incontiguous blocks
					contiguous_set<boost::uint64_t>::iterator inc_iter;
					inc_iter = iter->second.Contiguous.find(block_num);
					if(inc_iter == iter->second.Contiguous.end()){
						return DOWNLOADING_NOT_AVAILABLE;
					}else{
						return DOWNLOADING_AVAILABLE;
					}
				}
			}
		}else{
			//add_file_block() not yet called
			return DOWNLOADING_NOT_AVAILABLE;
		}
	}
}
