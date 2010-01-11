#include "block_request.hpp"

block_request::block_request(const boost::uint64_t block_count_in):
	block_count(block_count_in),
	local(block_count),
	approved(block_count)
{

}

void block_request::add_block_local(const boost::uint64_t block)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(!local.empty()){
		local[block] = true;
		if(local.all_set()){
			local.clear();
		}
	}
}

void block_request::add_block_local(const int connection_ID, const boost::uint64_t block)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	add_block_local(block);
	request.erase(block);
}

void block_request::add_block_local_all()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	local.clear();
	request.clear();
}

void block_request::add_block_remote(const int connection_ID, const boost::uint64_t block)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	//find bitset for remote host
	std::map<int, bit_field>::iterator remote_iter = remote.find(connection_ID);
	assert(remote_iter != remote.end());
	//add block
	if(!remote_iter->second.empty()){
		remote_iter->second[block] = true;
		if(remote_iter->second.all_set()){
			remote_iter->second.clear();
		}
	}
}

void block_request::add_host_complete(const int connection_ID)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	//insert empty bit_field (host has all blocks)
	std::pair<std::map<int, bit_field>::iterator, bool>
		ret = remote.insert(std::make_pair(connection_ID, bit_field(0)));
	assert(ret.second);
}

void block_request::add_host_incomplete(const int connection_ID, bit_field & BF)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	assert(BF.size() == block_count);
	std::pair<std::map<int, bit_field>::iterator, bool>
		ret = remote.insert(std::make_pair(connection_ID, BF));
	assert(ret.second);
}

void block_request::approve_block(const boost::uint64_t block)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(!approved.empty()){
		approved[block] = true;
		if(approved.all_set()){
			approved.clear();
		}
	}
}

void block_request::approve_block_all()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	approved.clear();
}

boost::uint64_t block_request::bytes()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return block_count % 8 == 0 ? block_count / 8 : block_count / 8 + 1;
}

bool block_request::complete()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	/*
	When the dynamic_bitset is complete it is clear()'d to save space. If a
	bit_field is clear we know the host has all blocks.
	*/
	return local.empty();
}

bool block_request::find_next_rarest(const int connection_ID, boost::uint64_t & block)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	//find bitset for remote host
	std::map<int, bit_field>::iterator remote_iter = remote.find(connection_ID);
	if(remote_iter == remote.end()){
		//we are waiting on a bit_field from the remote host most likely
		return false;
	}
	boost::uint64_t rare_block;           //most rare block
	boost::uint64_t rare_block_hosts = 0; //number of hosts that have rare_block
	for(block = 0; block < local.size(); ++block){
		if(local[block]){
			//we already have this block
			continue;
		}
		if(!approved.empty() && !approved[block]){
			//block is not approved
			continue;
		}
		//check rarity
		boost::uint32_t hosts = 0;
		for(std::map<int, bit_field>::iterator iter_cur = remote.begin(),
			iter_end = remote.end(); iter_cur != iter_end; ++iter_cur)
		{
			if(iter_cur->second.empty() || iter_cur->second[block]){
				++hosts;
			}
		}
		if(hosts == 1){
			//block with maximum rarity found
			if(request.find(block) == request.end()){
				//block not already requested
				return true;
			}
		}else if(hosts < rare_block_hosts || rare_block_hosts == 0){
			//a new most-rare block found
			if(request.find(block) == request.end()){
				//block not already requested, consider requesting this block
				rare_block = block;
				rare_block_hosts = hosts;
			}
		}
	}
	if(rare_block_hosts == 0){
		//host has no blocks we need
		return false;
	}else{
		//a block was found that we need
		block = rare_block;
		return true;
	}
}

bool block_request::have_block(const boost::uint64_t block)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	assert(block < block_count);
	if(complete()){
		return true;
	}else{
		return local[block] == true;
	}
}

bool block_request::is_approved(const boost::uint64_t block)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(approved.empty()){
		return true;
	}else{
		return approved[block] == true;
	}
}

bool block_request::next_request(const int connection_ID, boost::uint64_t & block)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(complete()){
		return false;
	}
	/*
	At this point we know there are no timed out requests to the host and there
	are no re-requests to do. We move on to checking for the next rarest block to
	request from the host.
	*/
	if(find_next_rarest(connection_ID, block)){
		//there is a new block to request
		std::pair<std::map<boost::uint64_t, std::set<int> >::iterator, bool>
			r_ret = request.insert(std::make_pair(block, std::set<int>()));
		assert(r_ret.second);
		std::pair<std::set<int>::iterator, bool>
			c_ret = r_ret.first->second.insert(connection_ID);
		assert(c_ret.second);
		return true;
	}else{
		/*
		No new blocks to request. If this connection has no requests pending then
		determine what block has been requested least, and make duplicate request.
		*/

		//determine if we have any requests pending
		for(std::map<boost::uint64_t, std::set<int> >::iterator
			iter_cur = request.begin(), iter_end = request.end();
			iter_cur != iter_end; ++iter_cur)
		{
			if(iter_cur->second.find(connection_ID) != iter_cur->second.end()){
				//pending request found, make no duplicate request
				return false;
			}
		}

		//determine what block has been requested least, and request it
		boost::uint64_t rare_block;
		unsigned min_request = std::numeric_limits<unsigned>::max();
		for(std::map<boost::uint64_t, std::set<int> >::iterator
			iter_cur = request.begin(), iter_end = request.end();
			iter_cur != iter_end; ++iter_cur)
		{
			if(iter_cur->second.size() == 1){
				//this block must be at least tied for least requested
				block = iter_cur->first;
				iter_cur->second.insert(connection_ID);
				return true;
			}else if(iter_cur->second.size() < min_request){
				rare_block = iter_cur->first;
				min_request = iter_cur->second.size();
			}
		}
		if(min_request != std::numeric_limits<unsigned>::max()){
			block = rare_block;
			std::map<boost::uint64_t, std::set<int> >::iterator
				iter = request.find(rare_block);
			assert(iter != request.end());
			iter->second.insert(connection_ID);
			return true;
		}
		return false;
	}
}

void block_request::remove_host(const int connection_ID)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	remote.erase(connection_ID);
	//erase request elements for this host
	std::map<boost::uint64_t, std::set<int> >::iterator
		iter_cur = request.begin(), iter_end = request.end();
	while(iter_cur != iter_end){
		iter_cur->second.erase(connection_ID);
		if(iter_cur->second.empty()){
			request.erase(iter_cur++);
		}else{
			++iter_cur;
		}
	}
}

unsigned block_request::remote_host_count()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return remote.size();
}
