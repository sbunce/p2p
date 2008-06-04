#include "download_file.h"

download_file::download_file(
	const std::string & file_hash_in,
	const std::string & file_name_in, 
	const std::string & file_path_in,
	const uint64_t & file_size_in,
	const uint64_t & latest_request_in,
	const uint64_t & last_block_in,
	const unsigned int & last_block_size_in
):
	file_hash(file_hash_in),
	file_name(file_name_in),
	file_path(file_path_in),
	file_size(file_size_in),
	last_block(last_block_in),
	last_block_size(last_block_size_in),
	download_complete(false),
	close_slots(false),
	Request_Gen(latest_request_in, last_block, global::RE_REQUEST_TIMEOUT)
{

}

bool download_file::complete()
{
	return download_complete;
}

const std::string & download_file::hash()
{
	return file_hash;
}

unsigned int download_file::max_response_size()
{
	return global::P_BLOCK_SIZE;
}

const std::string & download_file::name()
{
	return file_name;
}

unsigned int download_file::percent_complete()
{
	return (unsigned int)(((Request_Gen.highest_request() * global::P_BLOCK_SIZE)/(float)file_size)*100);
}

bool download_file::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected)
{
	download_file_conn * conn = (download_file_conn *)Connection[socket];

	if(!conn->slot_ID_requested){
		//slot_ID not yet obtained from server
		request = global::P_REQUEST_SLOT_FILE + hex::hex_to_binary(file_hash);
		conn->slot_ID_requested = true;
		expected.push_back(std::make_pair(global::P_SLOT_ID, global::P_SLOT_ID_SIZE));
		expected.push_back(std::make_pair(global::P_ERROR, global::P_ERROR_SIZE));
		return true;
	}
	if(!conn->slot_ID_received){
		//slot_ID requested but not yet received
		return false;
	}

	if(close_slots && !conn->close_slot_sent){
		/*
		The file finished downloading or the download was manually stopped. This
		server has not been sent P_CLOSE_SLOT.
		*/
		request += global::P_CLOSE_SLOT;
		request += conn->slot_ID;
		conn->close_slot_sent = true;
		//no response is expected for this command
	}

	if(close_slots){
		/*
		The download is complete when all servers have been sent a P_CLOSE_SLOT.
		*/
		bool unready_found = false;
		std::map<int, download_conn *>::iterator iter_cur, iter_end;
		iter_cur = Connection.begin();
		iter_end = Connection.end();
		while(iter_cur != iter_end){
			if(((download_file_conn *)iter_cur->second)->close_slot_sent == false){
				unready_found = true;
			}
			++iter_cur;
		}

		if(!unready_found){
			download_complete = true;
		}

		return false;
	}

	if(!Request_Gen.new_request(conn->latest_request)){
		//no request to be made at the moment
		return false;
	}

	request += global::P_SEND_BLOCK;
	request += conn->slot_ID;
	request += Convert_uint64.encode(conn->latest_request.back());
	int size;
	if(conn->latest_request.back() == last_block){
		size = last_block_size;
	}else{
		size = global::P_BLOCK_SIZE;
	}

	expected.push_back(std::make_pair(global::P_BLOCK, size));
	expected.push_back(std::make_pair(global::P_ERROR, 1));
	return true;
}

void download_file::response(const int & socket, std::string block)
{
	download_file_conn * conn = (download_file_conn *)Connection[socket];

	if(block[0] == global::P_SLOT_ID){
		conn->slot_ID = block[1];
		conn->slot_ID_received = true;
		return;
	}else if(block[0] == global::P_BLOCK){
		//a block was received
		block.erase(0, 1); //trim command
		conn->Speed_Calculator.update(block.size()); //update server speed
		Speed_Calculator.update(block.size());       //update download speed

//hash check should happen here
		if(!close_slots){
			write_block(conn->latest_request.front(), block);
		}

		Request_Gen.fulfilled(conn->latest_request.front());
		conn->latest_request.pop_front();

		if(Request_Gen.complete()){
			//download is complete, start closing slots
			close_slots = true;
		}
	}
}

void download_file::stop()
{
	namespace fs = boost::filesystem;
	close_slots = true;
	fs::path path = fs::system_complete(fs::path(file_path, fs::native));
	fs::remove(path);
}

void download_file::write_block(uint64_t block_number, std::string & block)
{
	std::fstream fout(file_path.c_str(), std::ios::in | std::ios::out | std::ios::binary);
	if(fout.is_open()){
		fout.seekp(block_number * (global::P_BLOCK_SIZE - 1), std::ios::beg);
		fout.write(block.c_str(), block.size());
	}else{
		logger::debug(LOGGER_P1,"error opening file ",file_path);
	}
}

const uint64_t & download_file::total_size()
{
	return file_size;
}
