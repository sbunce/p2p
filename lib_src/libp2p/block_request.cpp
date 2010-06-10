#include "block_request.hpp"

//BEGIN download_element
block_request::download_element::download_element(const bit_field & BF):
	block_BF(BF)
{

}

block_request::download_element::download_element(const download_element & DE):
	block_BF(DE.block_BF)
{

}
//END download_element

//BEGIN upload_element
block_request::upload_element::upload_element(const boost::function<void()> & trigger_tick_in):
	trigger_tick(trigger_tick_in)
{

}

block_request::upload_element::upload_element(const upload_element & UE):
	trigger_tick(UE.trigger_tick),
	block_diff(UE.block_diff)
{

}
//END upload_element

block_request::block_request(const boost::uint64_t block_count_in):
	block_count(block_count_in),
	local_blocks(0),
	local(block_count),
	approved(block_count)
{

}

void block_request::add_block_local(const boost::uint64_t block)
{
	boost::mutex::scoped_lock lock(Mutex);
	assert(Upload.empty());
	if(!local.empty()){
		assert(local[block] == false);
		local[block] = true;
		++local_blocks;
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
		++local_blocks;
		if(local.all_set()){
			local.clear();
		}
	}
	Request.erase(block);
	if(send_have){
		//we got new block, send have_* messages
		for(std::map<int, upload_element>::iterator u_it_cur = Upload.begin(),
			u_it_end = Upload.end(); u_it_cur != u_it_end; ++u_it_cur)
		{
			if(u_it_cur->first != connection_ID){
				std::map<int, download_element>::iterator d_it = Download.find(u_it_cur->first);
				if(d_it == Download.end()){
					//we can't know if host has block, send have_*
					u_it_cur->second.block_diff.push(block);
					u_it_cur->second.trigger_tick();
				}else{
					if(d_it->second.block_BF.empty() || d_it->second.block_BF[block] == false){
						//remote host doesn't have block
						u_it_cur->second.block_diff.push(block);
						u_it_cur->second.trigger_tick();
					}
				}
			}
		}
	}
}

void block_request::add_block_local_all()
{
	boost::mutex::scoped_lock lock(Mutex);
	local.clear();
	Request.clear();
	local_blocks = block_count;
}

void block_request::add_block_remote(const int connection_ID,
	const boost::uint64_t block)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, download_element>::iterator d_it = Download.find(connection_ID);
	assert(d_it != Download.end());
	if(!d_it->second.block_BF.empty()){
		d_it->second.block_BF[block] = true;
		if(d_it->second.block_BF.all_set()){
			d_it->second.block_BF.clear();
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

unsigned block_request::download_hosts()
{
	boost::mutex::scoped_lock lock(Mutex);
	return Download.size();
}

void block_request::download_reg(const int connection_ID,
	const bit_field & BF)
{
	boost::mutex::scoped_lock lock(Mutex);
	assert(BF.empty() || BF.size() == block_count);
	std::pair<std::map<int, download_element>::iterator, bool>
		ret = Download.insert(std::make_pair(connection_ID, download_element(BF)));
	assert(ret.second);
}

void block_request::download_unreg(const int connection_ID)
{
	boost::mutex::scoped_lock lock(Mutex);
	Download.erase(connection_ID);
	for(std::map<boost::uint64_t, std::set<int> >::iterator it_cur = Request.begin();
		it_cur != Request.end();)
	{
		it_cur->second.erase(connection_ID);
		if(it_cur->second.empty()){
			Request.erase(it_cur++);
		}else{
			++it_cur;
		}
	}
}

boost::optional<boost::uint64_t> block_request::find_next_rarest(const int connection_ID)
{
	//find bitset for remote host
	std::map<int, download_element>::iterator d_it = Download.find(connection_ID);
	if(d_it == Download.end()){
		//we are waiting on a bit_field from the remote host most likely
		return boost::optional<boost::uint64_t>();
	}
	boost::uint64_t rare_block;           //most rare block
	boost::uint64_t rare_block_hosts = 0; //number of hosts that have rare_block
	for(boost::uint64_t block = 0; block < local.size(); ++block){
		if(local[block]){
			//we already have this block
			continue;
		}
		if(!approved.empty() && !approved[block]){
			//block is not approved
			continue;
		}
		if(!d_it->second.block_BF.empty() && d_it->second.block_BF[block] == false){
			//remote host doesn't have this block
			continue;
		}

		//check rarity
		boost::uint32_t hosts = 0;
		for(std::map<int, download_element>::iterator it_cur = Download.begin(),
			it_end = Download.end(); it_cur != it_end; ++it_cur)
		{
			if(it_cur->second.block_BF.empty() || it_cur->second.block_BF[block]){
				++hosts;
			}
		}
		if(hosts == 1){
			//block with maximum rarity found
			if(Request.find(block) == Request.end()){
				//block not already requested
				return block;
			}
		}else if(hosts < rare_block_hosts || rare_block_hosts == 0){
			//a new most-rare block found
			if(Request.find(block) == Request.end()){
				//block not already requested, consider requesting this block
				rare_block = block;
				rare_block_hosts = hosts;
			}
		}
	}
	if(rare_block_hosts == 0){
		//host has no blocks we need
		return boost::optional<boost::uint64_t>();
	}else{
		//a block was found that we need
		return rare_block;
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

bool block_request::is_approved(const boost::uint64_t block)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(approved.empty()){
		return true;
	}else{
		return approved[block] == true;
	}
}

boost::optional<boost::uint64_t> block_request::next_have(const int connection_ID)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::map<int, upload_element>::iterator it = Upload.find(connection_ID);
	if(it == Upload.end()){
		return boost::optional<boost::uint64_t>();
	}else{
		if(it->second.block_diff.empty()){
			return boost::optional<boost::uint64_t>();
		}else{
			boost::uint64_t block = it->second.block_diff.front();
			it->second.block_diff.pop();
			return block;
		}
	}
}

boost::optional<boost::uint64_t> block_request::next_request(const int connection_ID)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(local.empty()){
		//complete
		return boost::optional<boost::uint64_t>();
	}
	/*
	At this point we know there are no timed out requests to the host and there
	are no re-requests to do. We move on to checking for the next rarest block to
	request from the host.
	*/
	if(boost::optional<boost::uint64_t> block = find_next_rarest(connection_ID)){
		//there is a new block to request
		std::pair<std::map<boost::uint64_t, std::set<int> >::iterator, bool>
			r_ret = Request.insert(std::make_pair(*block, std::set<int>()));
		assert(r_ret.second);
		std::pair<std::set<int>::iterator, bool> c_ret = r_ret.first->second.insert(connection_ID);
		assert(c_ret.second);
		return block;
	}else{
		/*
		No new blocks to request. If this connection has no requests pending then
		determine what block has been requested least, and make duplicate request.
		*/

		//determine if we have any requests pending
		for(std::map<boost::uint64_t, std::set<int> >::iterator
			it_cur = Request.begin(), it_end = Request.end();
			it_cur != it_end; ++it_cur)
		{
			if(it_cur->second.find(connection_ID) != it_cur->second.end()){
				//pending request found, make no duplicate request
				return boost::optional<boost::uint64_t>();
			}
		}

		//find the least requested block that the remote host has and request it
		std::map<int, download_element>::iterator d_it = Download.find(connection_ID);
		boost::uint64_t rare_block;
		unsigned min_request = std::numeric_limits<unsigned>::max();
		for(std::map<boost::uint64_t, std::set<int> >::iterator
			it_cur = Request.begin(), it_end = Request.end();
			it_cur != it_end; ++it_cur)
		{
			if(it_cur->second.size() == 1){
				//least requested possible, check if remote host has it
				if(d_it->second.block_BF.empty() || d_it->second.block_BF[it_cur->first] == true){
					it_cur->second.insert(connection_ID);
					return it_cur->first;
				}
			}else if(it_cur->second.size() < min_request){
				//new least requested block found, check if remote host has it
				if(d_it->second.block_BF.empty() || d_it->second.block_BF[it_cur->first] == true){
					rare_block = it_cur->first;
					min_request = it_cur->second.size();
				}
			}
		}
		if(min_request != std::numeric_limits<unsigned>::max()){
			std::map<boost::uint64_t, std::set<int> >::iterator it = Request.find(rare_block);
			assert(it != Request.end());
			it->second.insert(connection_ID);
			return rare_block;
		}else{
			return boost::optional<boost::uint64_t>();
		}
	}
}

unsigned block_request::percent_complete()
{
	boost::mutex::scoped_lock lock(Mutex);
	if(local.empty()){
		return 100;
	}else{
		return ((double)local_blocks / block_count) * 100;
	}
}

unsigned block_request::upload_hosts()
{
	boost::mutex::scoped_lock lock(Mutex);
	return Upload.size();
}

bit_field block_request::upload_reg(const int connection_ID,
	const boost::function<void()> trigger_tick)
{
	boost::mutex::scoped_lock lock(Mutex);
	//add have element
	std::pair<std::map<int, upload_element>::iterator, bool>
		p = Upload.insert(std::make_pair(connection_ID, upload_element(trigger_tick)));
	assert(p.second);
	return local;
}

void block_request::upload_unreg(const int connection_ID)
{
	boost::mutex::scoped_lock lock(Mutex);
	Upload.erase(connection_ID);
}
