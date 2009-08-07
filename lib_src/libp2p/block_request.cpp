#include "block_request.hpp"

block_request::block_request(
	const unsigned block_count_in
):
	block_count(block_count_in),
	local_block(block_count)
{
	local_block.set();
}

void block_request::add_block(const boost::uint32_t block)
{
	if(!local_block.empty()){
		local_block[block] = 0;
		if(local_block.none()){
			local_block.clear();
		}
	}
}

void block_request::add_block(const int socket_FD, const boost::uint32_t block)
{
	//find bitset for remote host
	std::map<int, boost::dynamic_bitset<> >::iterator
		remote_iter = remote_block.find(socket_FD);
	assert(remote_iter != remote_block.end());

	if(!remote_iter->second.empty()){
		remote_iter->second[block] = 0;
		if(remote_iter->second.none()){
			remote_iter->second.clear();
		}
	}
}

void block_request::add_host(const int socket_FD)
{
	remote_block.insert(std::make_pair(socket_FD, boost::dynamic_bitset<>(0)));
}

void block_request::add_host(const int socket_FD, boost::dynamic_bitset<> & BS)
{
	assert(BS.size() == block_count);
	remote_block.insert(std::make_pair(socket_FD, BS));
}

bool block_request::complete()
{
	/*
	When the dynamic_bitset is complete is is clear()'d to save space. After it
	has been cleared we know it is complete.
	*/
	return local_block.empty();
}

bool block_request::next(const int socket_FD, boost::uint32_t & block)
{
	assert(!complete());

	//find bitset for remote host
	std::map<int, boost::dynamic_bitset<> >::iterator
		remote_iter = remote_block.find(socket_FD);
	assert(remote_iter != remote_block.end());

	//find rarest block that we need, that remote host has
	block = 0;
	boost::uint32_t rare_block, rare_block_hosts = 0;
	bool checked_zero = false;
	while(true){
		if(block == 0 && !checked_zero){
			block = local_block.find_first();
			checked_zero = true;
		}else{
			block = local_block.find_next(block);
		}

		if(block == boost::dynamic_bitset<>::npos){
			break;
		}

		if(remote_iter->second.empty() || remote_iter->second[block] == 0){
			//remote host has block, check rarity
			boost::uint32_t hosts = 0;
			for(std::map<int, boost::dynamic_bitset<> >::iterator iter_cur = remote_block.begin(),
				iter_end = remote_block.end(); iter_cur != iter_end; ++iter_cur)
			{
				if(iter_cur->second.empty() || iter_cur->second[block] == 0){
					++hosts;
				}
			}

			if(hosts == 1){
				//block with max rarity found
				local_block[block] = 0;
				if(local_block.none()){
					local_block.clear();
				}
				return true;
			}else if(hosts < rare_block_hosts || rare_block_hosts == 0){
				//new rarest block, or no block yet found
				rare_block = block;
				rare_block_hosts = hosts;
			}
		}
	}

	if(rare_block_hosts != 0){
		local_block[rare_block] = 0;
		if(local_block.none()){
			local_block.clear();
		}
		block = rare_block;
		return true;
	}else{
		//host has no blocks we need
		return false;
	}
}

void block_request::remove_block(const boost::uint32_t block)
{
	if(local_block.empty()){
		local_block.resize(block_count);
	}
	local_block[block] = 1;
}

void block_request::remove_host(const int socket_FD)
{
	if(remote_block.erase(socket_FD) != 1){
		LOGGER << "violated precondition";
		exit(1);
	}
}

boost::uint32_t block_request::size()
{
	return block_count % 8 == 0 ? block_count / 8 : block_count / 8 + 1;
}
