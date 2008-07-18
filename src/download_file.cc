#include "download_file.h"

download_file::download_file(
	const std::string & file_hash_in,
	const std::string & file_name_in, 
	const std::string & file_path_in,
	const uint64_t & file_size_in
):
	file_hash(file_hash_in),
	file_name(file_name_in),
	file_path(file_path_in),
	file_size(file_size_in),
	hashing(true),
	hashing_percent(0),
	threads(0),
	stop_threads(false),
	thread_file_hash(file_hash_in),
	thread_file_path(file_path_in),
	download_complete(false),
	close_slots(false)
{
	/*
	Block requests start at zero so the lst block is the number of file block
	hashes there is minus one.
	*/
	if(file_size % global::FILE_BLOCK_SIZE == 0){
		//exact block count, subtract one to get last_block number
		last_block = file_size / global::FILE_BLOCK_SIZE - 1;
	}else{
		//partial last block (decimal gets truncated which is effectively minus one)
		last_block = file_size / global::FILE_BLOCK_SIZE;
	}

	/*
	If the file size is exactly divisible by global::FILE_BLOCK_SIZE there is no
	partial last block.
	*/
	if(file_size % global::FILE_BLOCK_SIZE == 0){
		//last block exactly global::FILE_BLOCKS_SIZE + command size
		last_block_size = global::FILE_BLOCK_SIZE + 1;
	}else{
		//partial last block + command size
		last_block_size = file_size % global::FILE_BLOCK_SIZE + 1;
	}

	std::fstream fin((global::DOWNLOAD_DIRECTORY+file_name).c_str(), std::ios::in);
	assert(fin.is_open());
	fin.seekg(0, std::ios::end);
	first_unreceived = fin.tellg() / global::FILE_BLOCK_SIZE;

	//start re_requesting where the download left off
	Request_Gen.init((uint64_t)first_unreceived, last_block, global::RE_REQUEST_TIMEOUT);

	//hash check for corrupt/missing blocks
	boost::thread T(boost::bind(&download_file::hash_check, this));
}

download_file::~download_file()
{
	stop_threads = true;
	while(threads){
		usleep(1);
	}
}

bool download_file::complete()
{
	return download_complete;
}

const std::string download_file::hash()
{
	return file_hash;
}

void download_file::hash_check()
{
	++threads;
	/*
	This is done so that on resume all the downloads can be added quickly before
	things bog down with hash checking.
	*/
	sleep(1);

	std::fstream fin(thread_file_path.c_str(), std::ios::in);
	char block_buff[global::FILE_BLOCK_SIZE];
	uint64_t hash_latest = 0;
	while(true){
		if(hash_latest == first_unreceived){
			break;
		}
		fin.read(block_buff, global::FILE_BLOCK_SIZE);
		if(!Hash_Tree.check_block(thread_file_hash, hash_latest, block_buff, fin.gcount())){
			logger::debug(LOGGER_P1,"found corrupt block ",hash_latest," in resumed download");
			Request_Gen.force_re_request(hash_latest);
		}
		++hash_latest;

		hashing_percent = (int)(((double)hash_latest / first_unreceived) * 100);

		if(stop_threads){
			break;
		}
	}
	hashing = false;
	--threads;
}

const std::string download_file::name()
{
	if(hashing){
		std::ostringstream name;
		name << file_name + " CHECK " << hashing_percent << "%";
		return name.str();
	}else{
		return file_name;
	}
}

unsigned int download_file::percent_complete()
{
	if(last_block == 0){
		return 0;
	}else{
		return (unsigned int)(((float)Request_Gen.highest_requested() / (float)last_block)*100);
	}
}

void download_file::register_connection(const download_connection & DC)
{
	download::register_connection(DC);
	Connection_Special.insert(std::make_pair(DC.socket, connection_special(DC.IP)));
}

download::mode download_file::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected)
{
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	assert(iter != Connection_Special.end());
	connection_special * conn = &iter->second;

	if(conn->close_slot_sent || conn->abusive){
		return download::NO_REQUEST;
	}

	if(!conn->slot_ID_requested){
		//slot_ID not yet obtained from server
		request = global::P_REQUEST_SLOT_FILE + convert::hex_to_binary(file_hash);
		conn->slot_ID_requested = true;
		expected.push_back(std::make_pair(global::P_SLOT_ID, global::P_SLOT_ID_SIZE));
		expected.push_back(std::make_pair(global::P_ERROR, 1));
		return download::BINARY_MODE;
	}else if(!conn->slot_ID_received){
		//slot_ID requested but not yet received
		return download::NO_REQUEST;
	}

	if(close_slots && !conn->close_slot_sent){
		/*
		The file finished downloading or the download was manually stopped. This
		server has not been sent P_CLOSE_SLOT.
		*/
		request += global::P_CLOSE_SLOT;
		request += conn->slot_ID;
		conn->close_slot_sent = true;
		bool unready_found = false;
		std::map<int, connection_special>::iterator iter_cur, iter_end;
		iter_cur = Connection_Special.begin();
		iter_end = Connection_Special.end();
		while(iter_cur != iter_end){
			if(iter_cur->second.close_slot_sent == false){
				unready_found = true;
			}
			++iter_cur;
		}
		if(!unready_found){
			download_complete = true;
		}
		return download::BINARY_MODE;
	}

	if(download_complete){
		return download::NO_REQUEST;
	}

	if(!Request_Gen.complete() && Request_Gen.request(conn->requested_blocks)){
		//prepare request for needed block
		request += global::P_SEND_BLOCK;
		request += conn->slot_ID;
		request += convert::encode<uint64_t>(conn->requested_blocks.back());
		int size;
		if(conn->requested_blocks.back() == last_block){
			size = last_block_size;
		}else{
			size = global::P_BLOCK_SIZE;
		}
		expected.push_back(std::make_pair(global::P_BLOCK, size));
		expected.push_back(std::make_pair(global::P_ERROR, 1));
		return download::BINARY_MODE;
	}

	return download::NO_REQUEST;
}

void download_file::response(const int & socket, std::string block)
{
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	assert(iter != Connection_Special.end());
	connection_special * conn = &iter->second;

	if(conn->abusive){
		return;
	}

	if(block[0] == global::P_SLOT_ID && conn->slot_ID_received == false){
		conn->slot_ID = block[1];
		conn->slot_ID_received = true;
	}else if(block[0] == global::P_BLOCK){
		Speed_Calculator.update(block.size()); //update download speed
		block.erase(0, 1);                     //trim command
		if(!Hash_Tree.check_block(file_hash, conn->requested_blocks.front(), block.c_str(), block.length())){
			logger::debug(LOGGER_P1,file_name,":",conn->requested_blocks.front()," hash failure");
			Request_Gen.force_re_request(conn->requested_blocks.front());
			DB_blacklist::add(conn->IP);
			conn->abusive = true;
		}else{
			write_block(conn->requested_blocks.front(), block);
			Request_Gen.fulfil(conn->requested_blocks.front());
		}
		conn->requested_blocks.pop_front();
		if(Request_Gen.complete() && !hashing){
			//download is complete, start closing slots
			close_slots = true;
		}
	}else if(block[0] == global::P_ERROR){
		logger::debug(LOGGER_P1,"received P_ERROR from ",conn->IP);
	}
}

const uint64_t download_file::size()
{
	return file_size;
}

void download_file::stop()
{
	namespace fs = boost::filesystem;
	fs::path path = fs::system_complete(fs::path(file_path, fs::native));
	fs::remove(path);
	if(Connection.size() == 0){
		download_complete = true;
	}else{
		close_slots = true;
	}
}

void download_file::write_block(uint64_t block_number, std::string & block)
{
	std::fstream fout(file_path.c_str(), std::ios::in | std::ios::out | std::ios::binary);
	if(fout.is_open()){
		fout.seekp(block_number * global::FILE_BLOCK_SIZE, std::ios::beg);
		fout.write(block.c_str(), block.size());
	}else{
		logger::debug(LOGGER_P1,"error opening file ",file_path);
	}
}

void download_file::unregister_connection(const int & socket)
{
	//re_request all blocks that are pending for the server that's getting disconnected
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	if(iter != Connection_Special.end()){
		std::deque<uint64_t>::iterator RB_iter_cur, RB_iter_end;
		RB_iter_cur = iter->second.requested_blocks.begin();
		RB_iter_end = iter->second.requested_blocks.end();
		while(RB_iter_cur != RB_iter_end){
			Request_Gen.force_re_request(*RB_iter_cur);
			++RB_iter_cur;
		}
	}

	download::unregister_connection(socket);
	Connection_Special.erase(socket);
}

bool download_file::visible()
{
	return true;
}
