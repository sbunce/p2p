#include "slot_file.hpp"

slot_file::slot_file(
	std::string * IP_in,
	const std::string & root_hash_in,
	const std::string & path_in,
	const boost::uint64_t & file_size_in
):
	path(path_in),
	block_count(0),
	percent(0),
	highest_block_seen(0)
{
	IP = IP_in;
	root_hash = root_hash_in;
	file_size = file_size_in;

	if(file_size % protocol::FILE_BLOCK_SIZE == 0){
		//exact block count, subtract one to get block_count number
		block_count = file_size / protocol::FILE_BLOCK_SIZE;
	}else{
		//partial last block (decimal gets truncated which is effectively minus one)
		block_count = file_size / protocol::FILE_BLOCK_SIZE + 1;
	}

	#ifdef CORRUPT_FILE_BLOCKS
	std::srand(time(NULL));
	#endif
}

void slot_file::info(std::vector<upload_info> & UI)
{
	UI.push_back(upload_info(
		root_hash,
		*IP,
		file_size,
		path,
		Speed_Calculator.speed(),
		percent
	));
}

bool slot_file::read_block(const boost::uint64_t & block_num, std::string & send)
{
	std::fstream fin(path.c_str(), std::ios::in | std::ios::binary);
	if(fin.is_open()){
		//seek to the file_block the client wants (-1 for command space)
		fin.seekg(block_num * protocol::FILE_BLOCK_SIZE);
		fin.read(send_block_buff, protocol::FILE_BLOCK_SIZE);
		#ifdef CORRUPT_FILE_BLOCK_TEST
		if(std::rand() % 5 == 0){
			LOGGER << "CORRUPT FILE BLOCK TEST, block " << block_num << " -> " << *IP;
			send_block_buff[0] = ~send_block_buff[0];
		}
		#endif
		send += protocol::P_BLOCK;
		send.append(send_block_buff, fin.gcount());
		Speed_Calculator.update(send.size());
		update_percent(block_num);
		return true;
	}else{
		return false;
	}
}

void slot_file::send_block(const std::string & request, std::string & send)
{
	boost::uint64_t block_num = convert::decode<boost::uint64_t>(request.substr(2, 8));
	if(block_num >= block_count){
		//client requested block higher than last block
		LOGGER << *IP << " requested block higher than maximum, blacklist";
		DB_Blacklist.add(*IP);
		return;
	}

	block_arbiter::download_state DS = block_arbiter::singleton().file_block_available(root_hash, block_num);
	if(DS == block_arbiter::DOWNLOADING_NOT_AVAILABLE){
		LOGGER << "sending P_WAIT to " << *IP;
		send += protocol::P_WAIT;
		Speed_Calculator.update(protocol::P_WAIT_SIZE);
	}else{
		if(!read_block(block_num, send)){
			//could not read file
			LOGGER << "server could not open file: " << path;
			send += protocol::P_ERROR;
			Speed_Calculator.update(protocol::P_ERROR_SIZE);
		}
	}
}

void slot_file::update_percent(const boost::uint64_t & latest_block)
{
	if(latest_block != 0 && latest_block > highest_block_seen){
		percent = (int)(((double)latest_block / (double)block_count)*100);
		highest_block_seen = latest_block;
		assert(percent <= 100);
	}
}
