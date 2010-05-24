#include "block_request.hpp"

block_request::block_request(const boost::uint64_t block_count_in):
	block_count(block_count_in),
	local(block_count),
	approved(block_count)
{

}

void block_request::add_block_local(const boost::uint64_t block)
{
	boost::mutex::scoped_lock lock(Mutex);
	assert(have.empty());
	if(!local.empty()){
		assert(local[block] == false);
		local[block] = true;
		if(local.all_set()){
			local.clear();
		}
	}
}

void block_request::add_block_local(const int connection_ID, const boost::uint64_t block)
{
	boost::mutex::scoped_lock lock(Mutex);
	bool send_have = false;
	if(!local.empty()){
		if(local[block] == false){
			send_have = true;
		}
		local[block] = true;
		if(local.all_set()){
			local.clear();
		}
	}
	request.erase(block);
	if(send_have){
		/*
		We don't tell the host that sent us a block that we have the block. We
		also don't tell it we have blocks we know it already has.
		*/
		std::set<int> trigger;
		for(std::map<int, have_info>::iterator it_cur = have.begin(),
			it_end = have.end(); it_cur != it_end; ++it_cur)
		{
			if(it_cur->first != connection_ID){
				std::map<int, bit_field>::iterator iter = remote.find(it_cur->first);
				if(iter == remote.end()){
					/*
					We are downloading from remote host, but it is not downoading from
					us so we don't know what blocks it has.
					*/
					it_cur->second.block.push(block);
					trigger.insert(it_cur->first);
				}else{
					/*
					We are downloading from remote host, so we can check to see if it
					already has a block. If it does there's no need to tell it we have
					the block because it won't need to request it from us.
					*/
					if(iter->second.empty() || iter->second[block] == false){
						//remote host doesn't have block
						it_cur->second.block.push(block);
						trigger.insert(it_cur->first);
					}
				}
			}
		}
		for(std::map<int, have_info>::iterator it_cur = have.begin(),
			it_end = have.end(); it_cur != it_end; ++it_cur)
		{
			if(it_cur->first != connection_ID
				&& trigger.find(it_cur->first) != trigger.end())
			{
				it_cur->second.trigger_tick(it_cur->first);
			}
		}
	}
}

void block_request::add_block_local_all()
{
	boost::mutex::scoped_lock lock(Mutex);
	local.clear();
	request.clear();
}

void block_request::add_block_remote(const int connection_ID,
	const boost::uint64_t block)
{
	boost::mutex::scoped_lock lock(Mutex);
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

void block_request::approve_block(const boost::uint64_t block)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(!approved.empty()){
		approved[block] = true;
		if(approved.all_set()){
			approved.clear();
		}
	}
}

void block_request::approve_block_all()
{
	boost::mutex::scoped_lock lock(Mutex);
	approved.clear();
}

boost::uint64_t block_request::bytes()
{
	return block_count % 8 == 0 ? block_count / 8 : block_count / 8 + 1;
}

bool block_request::complete()
{
	boost::mutex::scoped_lock lock(Mutex);
	/*
	When the dynamic_bitset is complete it is clear()'d to save space. If a
	bit_field is clear we know the host has all blocks.
	*/
	return local.empty();
}

bool block_request::find_next_rarest(const int connection_ID, boost::uint64_t & block)
{
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
		if(!remote_iter->second.empty() && remote_iter->second[block] == false){
			//remote host doesn't have this block
			continue;
		}

		//check rarity
		boost::uint32_t hosts = 0;
		for(std::map<int, bit_field>::iterator it_cur = remote.begin(),
			it_end = remote.end(); it_cur != it_end; ++it_cur)
		{
			if(it_cur->second.empty() || it_cur->second[block]){
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
	boost::mutex::scoped_lock lock(Mutex);
	assert(block < block_count);
	if(local.empty()){
		//complete
		return true;
	}else{
		return local[block] == true;
	}
}

unsigned block_request::incoming_count()
{
	boost::mutex::scoped_lock lock(Mutex);
	return have.size();
}

void block_request::incoming_subscribe(const int connection_ID,
	const boost::function<void(const int)> trigger_tick, bit_field & BF)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(local.empty()){
		BF.clear();
	}else{
		std::pair<std::map<int, have_info>::iterator, bool>
			ret = have.insert(std::make_pair(connection_ID, have_info()));
		assert(ret.second);
		ret.first->second.trigger_tick = trigger_tick;
		BF = local;
	}
}

void block_request::incoming_unsubscribe(const int connection_ID)
{
	boost::mutex::scoped_lock lock(Mutex);
	have.erase(connection_ID);
}

bool block_request::is_approved(const boost::uint64_t block)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(approved.empty()){
		return true;
	}else{
		return approved[block] == true;
	}
}

unsigned block_request::outgoing_count()
{
	boost::mutex::scoped_lock lock(Mutex);
	return remote.size();
}

void block_request::outgoing_subscribe(const int connection_ID,
	bit_field & BF)
{
	boost::mutex::scoped_lock lock(Mutex);
	assert(BF.empty() || BF.size() == block_count);
	std::pair<std::map<int, bit_field>::iterator, bool>
		ret = remote.insert(std::make_pair(connection_ID, BF));
	assert(ret.second);
}

void block_request::outgoing_unsubscribe(const int connection_ID)
{
	boost::mutex::scoped_lock lock(Mutex);
	remote.erase(connection_ID);
	//erase request elements for this host
	std::map<boost::uint64_t, std::set<int> >::iterator
		it_cur = request.begin(), it_end = request.end();
	while(it_cur != it_end){
		it_cur->second.erase(connection_ID);
		if(it_cur->second.empty()){
			request.erase(it_cur++);
		}else{
			++it_cur;
		}
	}
}

bool block_request::next_have(const int connection_ID, boost::uint64_t & block)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, have_info>::iterator iter = have.find(connection_ID);
	if(iter == have.end()){
		return false;
	}else{
		if(iter->second.block.empty()){
			return false;
		}else{
			block = iter->second.block.front();
			iter->second.block.pop();
			return true;
		}
	}
}

bool block_request::next_request(const int connection_ID, boost::uint64_t & block)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(local.empty()){
		//complete
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
			it_cur = request.begin(), it_end = request.end();
			it_cur != it_end; ++it_cur)
		{
			if(it_cur->second.find(connection_ID) != it_cur->second.end()){
				//pending request found, make no duplicate request
				return false;
			}
		}

		//find the least requested block that the remote host has and request it
		std::map<int, bit_field>::iterator remote_iter = remote.find(connection_ID);
		boost::uint64_t rare_block;
		unsigned min_request = std::numeric_limits<unsigned>::max();
		for(std::map<boost::uint64_t, std::set<int> >::iterator
			it_cur = request.begin(), it_end = request.end();
			it_cur != it_end; ++it_cur)
		{
			if(it_cur->second.size() == 1){
				//least requested possible, check if remote host has it
				if(remote_iter->second.empty() || remote_iter->second[it_cur->first] == true){
					block = it_cur->first;
					it_cur->second.insert(connection_ID);
					return true;
				}
			}else if(it_cur->second.size() < min_request){
				//new least requested block found, check if remote host has it
				if(remote_iter->second.empty() || remote_iter->second[it_cur->first] == true){
					rare_block = it_cur->first;
					min_request = it_cur->second.size();
				}
			}
		}
		if(min_request != std::numeric_limits<unsigned>::max()){
			block = rare_block;
			std::map<boost::uint64_t, std::set<int> >::iterator
				iter = request.find(rare_block);
			assert(iter != request.end());
			iter->second.insert(connection_ID);
			return true;
		}else{
			return false;
		}
	}
}

unsigned block_request::remote_host_count()
{
	boost::mutex::scoped_lock lock(Mutex);
	return remote.size();
}
