#include "slot_file.h"

slot_file::slot_file(
	std::string * IP_in,
	const std::string & root_hash_in,
	const boost::uint64_t & file_size_in,
	const std::string & path_in
):
	path(path_in),
	block_count(0),
	percent(0),
	highest_block_seen(0)
{
	IP = IP_in;
	root_hash = root_hash_in;
	file_size = file_size_in;

	if(file_size % global::FILE_BLOCK_SIZE == 0){
		//exact block count, subtract one to get block_count number
		block_count = file_size / global::FILE_BLOCK_SIZE;
	}else{
		//partial last block (decimal gets truncated which is effectively minus one)
		block_count = file_size / global::FILE_BLOCK_SIZE + 1;
	}
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

void slot_file::send_block(const std::string & request, std::string & send)
{
	boost::uint64_t block_number = convert::decode<boost::uint64_t>(request.substr(2, 8));
	client_server_bridge::download_mode DM = client_server_bridge::is_downloading(root_hash);
	if(DM == client_server_bridge::DOWNLOAD_FILE){
		//client requested a block from a file the client is currently downloading
		if(!client_server_bridge::is_available(root_hash, block_number)){
			//block is not available to be sent, tell the client to wait
			logger::debug(LOGGER_P1,"sending P_WAIT to ",*IP);
			send += global::P_WAIT;
			Speed_Calculator.update(global::P_WAIT_SIZE);
			return;
		}
	}

	std::fstream fin(path.c_str(), std::ios::in | std::ios::binary);
	if(fin.is_open()){
		//seek to the file_block the client wants (-1 for command space)
		fin.seekg(block_number * global::FILE_BLOCK_SIZE);
		fin.read(send_block_buff, global::FILE_BLOCK_SIZE);
		#ifdef CORRUPT_BLOCKS
		if(rand() % 100 == 0){
			send_block_buff[0] = ~send_block_buff[0];
		}
		#endif
		send += global::P_BLOCK;
		send.append(send_block_buff, fin.gcount());
		Speed_Calculator.update(send.size());
		update_percent(block_number);
	}else{
		//file not found in share, error
		logger::debug(LOGGER_P1,"server could not open file: ",path);
		send += global::P_ERROR;
		Speed_Calculator.update(global::P_ERROR_SIZE);
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
