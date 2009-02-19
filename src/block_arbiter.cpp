#include "block_arbiter.hpp"

//BEGIN STATIC
void block_arbiter::start_hash_tree(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex());
	Block_Arbiter().start_hash_tree_priv(hash);
}

void block_arbiter::start_file(const std::string & hash, const boost::uint64_t & file_block_count)
{
	boost::mutex::scoped_lock lock(Mutex());
	Block_Arbiter().start_file_priv(hash, file_block_count);
}

void block_arbiter::finish_download(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex());
	Block_Arbiter().finish_download_priv(hash);
}

void block_arbiter::update_hash_tree_highest(const std::string & hash, const boost::uint64_t & block_num)
{
	boost::mutex::scoped_lock lock(Mutex());
	Block_Arbiter().update_hash_tree_highest_priv(hash, block_num);
}

void block_arbiter::add_file_block(const std::string & hash, const boost::uint64_t & block_num)
{
	boost::mutex::scoped_lock lock(Mutex());
	Block_Arbiter().add_file_block_priv(hash, block_num);
}

bool block_arbiter::is_downloading(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex());
	return Block_Arbiter().is_downloading_priv(hash);
}

block_arbiter::download_state block_arbiter::hash_block_available(const std::string & hash, const boost::uint64_t & block_num)
{
	boost::mutex::scoped_lock lock(Mutex());
	return Block_Arbiter().hash_block_available_priv(hash, block_num);
}

block_arbiter::download_state block_arbiter::file_block_available(const std::string & hash, const boost::uint64_t & block_num)
{
	boost::mutex::scoped_lock lock(Mutex());
	return Block_Arbiter().file_block_available_priv(hash, block_num);
}

boost::mutex & block_arbiter::Mutex()
{
	static boost::mutex * M = new boost::mutex();
	return *M;
}

block_arbiter & block_arbiter::Block_Arbiter()
{
	/*
	The block arbiter singleton is called by the download_hash_tree and
	download_file dtors when the static containers within p2p_buffer delete the
	last boost::share_ptr references from themselves upon destruction. Because
	it's undefined whether or not the block arbiter will be destructed before
	the static containers within the p2p buffer, the block_arbiter is new'd so
	it is never destructed. But that's OK because nothing needs to be done upon
	destruction in the block_arbiter anyways.

	Holy hell static initialization order is complicated..
	*/
	static block_arbiter * B = new block_arbiter();
	return *B;
}
//END STATIC

block_arbiter::block_arbiter()
{

}

block_arbiter::~block_arbiter()
{
	//WARNING: THIS IS NEVER CALLED, see Block_Arbiter() documentation
}

void block_arbiter::start_hash_tree_priv(const std::string & hash)
{
	//assert file is not started, hash tree must download before file
	assert(File_State.find(hash) == File_State.end());
	Hash_Tree_State.insert(std::make_pair(hash, hash_tree_state()));
}

void block_arbiter::start_file_priv(const std::string & hash, const boost::uint64_t & file_block_count)
{
	Hash_Tree_State.erase(hash);
	File_State.insert(std::make_pair(hash, file_state(file_block_count)));
}

void block_arbiter::finish_download_priv(const std::string & hash)
{
	Hash_Tree_State.erase(hash);
	File_State.erase(hash);
}

void block_arbiter::update_hash_tree_highest_priv(const std::string & hash, const boost::uint64_t & block_num)
{
	std::map<std::string, hash_tree_state>::iterator iter;
	iter = Hash_Tree_State.find(hash);
	assert(iter != Hash_Tree_State.end());
	iter->second.initialized = true;
	iter->second.highest_available = block_num;
}

void block_arbiter::add_file_block_priv(const std::string & hash, const boost::uint64_t & block_num)
{
	std::map<std::string, file_state>::iterator iter;
	iter = File_State.find(hash);
	assert(iter != File_State.end());
	iter->second.initialized = true;
	iter->second.Contiguous.insert(block_num);
	iter->second.Contiguous.trim_contiguous();
}

bool block_arbiter::is_downloading_priv(const std::string & hash)
{
	std::map<std::string, hash_tree_state>::iterator h_iter;
	h_iter = Hash_Tree_State.find(hash);
	std::map<std::string, file_state>::iterator f_iter;
	f_iter = File_State.find(hash);
	return h_iter != Hash_Tree_State.end() || f_iter != File_State.end();
}

block_arbiter::download_state block_arbiter::hash_block_available_priv(const std::string & hash, const boost::uint64_t & block_number)
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

block_arbiter::download_state block_arbiter::file_block_available_priv(const std::string & hash, const boost::uint64_t & block_num)
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
