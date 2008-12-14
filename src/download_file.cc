#include "download_file.h"

download_file::download_file(
	const std::string & root_hash_hex_in,
	const std::string & file_name_in, 
	const std::string & file_path_in,
	const boost::uint64_t & file_size_in
):
	root_hash_hex(root_hash_hex_in),
	file_name(file_name_in),
	file_path(file_path_in),
	file_size(file_size_in),
	hashing(true),
	hashing_percent(0),
	thread_root_hash_hex(root_hash_hex_in),
	thread_file_path(file_path_in),
	download_complete(false),
	close_slots(false),
	_cancel(false),
	_visible(true),
	Tree_Info(root_hash_hex, file_size)
{
	client_server_bridge::transition_download(root_hash_hex);

	if(file_size % global::FILE_BLOCK_SIZE == 0){
		//exact block count, subtract one to get block_count number
		block_count = file_size / global::FILE_BLOCK_SIZE;
	}else{
		//partial last block (decimal gets truncated which is effectively minus one)
		block_count = file_size / global::FILE_BLOCK_SIZE + 1;
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

	std::fstream fin((global::DOWNLOAD_DIRECTORY+file_name).c_str(), std::ios::in | std::ios::ate);
	assert(fin.is_open());
	first_unreceived = fin.tellg() / global::FILE_BLOCK_SIZE;

	//start re_requesting where the download left off
	Request_Generator.init((boost::uint64_t)first_unreceived, block_count, global::RE_REQUEST);

	//hash check for corrupt/missing blocks
	hashing_thread = boost::thread(boost::bind(&download_file::hash_check, this, Tree_Info));
}

download_file::~download_file()
{
	hashing_thread.interrupt();
	hashing_thread.join();

	if(!_cancel){
		DB_Share.add_entry(root_hash_hex, file_size, file_path);
	}

	client_server_bridge::finish_download(root_hash_hex);
}

bool download_file::complete()
{
	return download_complete;
}

const std::string download_file::hash()
{
	return root_hash_hex;
}

void download_file::hash_check(const hash_tree::tree_info & Tree_Info)
{
	std::fstream fin(thread_file_path.c_str(), std::ios::in);
	assert(fin.good());
	char block_buff[global::FILE_BLOCK_SIZE];
	boost::uint64_t hash_latest = 0;
	while(true){
		boost::this_thread::interruption_point();

		if(hash_latest == first_unreceived){
			break;
		}

		fin.read(block_buff, global::FILE_BLOCK_SIZE);
		assert(fin.good());

		if(!Hash_Tree.check_file_block(Tree_Info, hash_latest, block_buff, fin.gcount())){
			logger::debug(LOGGER_P1,"found corrupt block ",hash_latest," in resumed download");
			Request_Generator.force_re_request(hash_latest);
		}
		++hash_latest;

		hashing_percent = (int)(((double)hash_latest / first_unreceived) * 100);
	}
	hashing = false;
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
	if(block_count == 0){
		return 0;
	}else{
		return (unsigned int)(((float)Request_Generator.highest_requested() / (float)block_count)*100);
	}
}

void download_file::register_connection(const download_connection & DC)
{
	download::register_connection(DC);
	Connection_Special.insert(std::make_pair(DC.socket, connection_special(DC.IP)));
}

download::mode download_file::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected, int & slots_used)
{
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	assert(iter != Connection_Special.end());
	connection_special * conn = &iter->second;

	if(conn->State == connection_special::ABUSIVE){
		//server abusive but not yet disconnected
		return download::NO_REQUEST;
	}else if(conn->State == connection_special::REQUEST_SLOT){
		//slot_ID not yet obtained from server
		if(close_slots){
			//download is in process of closing slots, don't request a slot
			conn->State = connection_special::CLOSED_SLOT;
			return download::NO_REQUEST;
		}
		if(slots_used >= 255){
			//the server has no free slots left, wait for one to free up
			return download::NO_REQUEST;
		}

		//slot available, make slot request
		request = global::P_REQUEST_SLOT_FILE + convert::hex_to_binary(root_hash_hex);
		conn->State = connection_special::AWAITING_SLOT;
		++slots_used;
		expected.push_back(std::make_pair(global::P_SLOT_ID, global::P_SLOT_ID_SIZE));
		expected.push_back(std::make_pair(global::P_ERROR, global::P_ERROR_SIZE));
		return download::BINARY_MODE;
	}else if(conn->State == connection_special::AWAITING_SLOT){
		//wait until slot_ID is received
		return download::NO_REQUEST;
	}else if(conn->State == connection_special::REQUEST_BLOCKS){
		if(close_slots){
			/*
			The file finished downloading or the download was manually stopped. This
			server has not been sent P_CLOSE_SLOT.
			*/
			request += global::P_CLOSE_SLOT;
			request += conn->slot_ID;
			--slots_used;
			conn->State = connection_special::CLOSED_SLOT;

			std::map<int, connection_special>::iterator iter_cur, iter_end;
			iter_cur = Connection_Special.begin();
			iter_end = Connection_Special.end();
			bool unready_found = false;
			while(iter_cur != iter_end){
				if(iter_cur->second.State != connection_special::CLOSED_SLOT){
					unready_found = true;
					break;
				}
				++iter_cur;
			}
			if(!unready_found){
				download_complete = true;
			}
			return download::BINARY_MODE;
		}

		if(download_complete){
			//all blocks received, no need to make any more requests
			return download::NO_REQUEST;
		}

		if(conn->wait_activated){
			//check to see if wait is completed
			if(conn->wait_start + global::P_WAIT_TIMEOUT <= time(NULL)){
				//timeout expired
				conn->wait_activated = false;
			}else{
				//timeout not yet expired
				return download::NO_REQUEST;
			}
		}

		if(!Request_Generator.complete() && Request_Generator.request(conn->latest_request)){
			//prepare request for needed block
			request += global::P_BLOCK;
			request += conn->slot_ID;
			request += convert::encode<boost::uint64_t>(conn->latest_request.back());
			int size;
			if(conn->latest_request.back() == block_count - 1){
				size = last_block_size;
				Request_Generator.set_timeout(global::RE_REQUEST_FINISHING);
			}else{
				size = global::P_BLOCK_TO_CLIENT_SIZE;
			}
			expected.push_back(std::make_pair(global::P_BLOCK, size));
			expected.push_back(std::make_pair(global::P_ERROR, global::P_ERROR_SIZE));
			expected.push_back(std::make_pair(global::P_WAIT, global::P_WAIT_SIZE));
			return download::BINARY_MODE;
		}else{
			//no more requests to make
			return download::NO_REQUEST;
		}
	}else if(conn->State == connection_special::CLOSED_SLOT){
		//slot closed, this download is done with the server
		return download::NO_REQUEST;
	}

	logger::debug(LOGGER_P1,"logic error: unhandled case");
	exit(1);
}

void download_file::response(const int & socket, std::string block)
{
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	assert(iter != Connection_Special.end());
	connection_special * conn = &iter->second;

	if(conn->State == connection_special::ABUSIVE){
		//abusive but not yet disconnected, ignore data
		return;
	}else if(conn->State == connection_special::AWAITING_SLOT){
		if(block[0] == global::P_SLOT_ID){
			//received slot, ready to request blocks
			conn->slot_ID = block[1];
			conn->State = connection_special::REQUEST_BLOCKS;
		}else if(block[0] == global::P_ERROR){
			logger::debug(LOGGER_P1,"server ",conn->IP," does not have file, REMOVAL FROM DB NOT IMPLEMENTED");
		}else{
			logger::debug(LOGGER_P1,"logic error: unhandled case");
			exit(1);
		}
		return;
	}else if(conn->State == connection_special::REQUEST_BLOCKS){
		if(block[0] == global::P_BLOCK){
			block.erase(0, 1); //trim command
			if(Hash_Tree.check_file_block(Tree_Info, conn->latest_request.front(), block.c_str(), block.length())){
				write_block(conn->latest_request.front(), block);
				client_server_bridge::download_block_received(root_hash_hex, conn->latest_request.front());
				Request_Generator.fulfil(conn->latest_request.front());
			}else{
				logger::debug(LOGGER_P1,file_name,":",conn->latest_request.front()," hash failure");
				Request_Generator.force_re_request(conn->latest_request.front());
				DB_blacklist::add(conn->IP);
				conn->State = connection_special::ABUSIVE;
			}
			conn->latest_request.pop_front();
			if(Request_Generator.complete() && !hashing){
				//download is complete, start closing slots
				close_slots = true;
			}
		}else if(block[0] == global::P_WAIT){
			//server doesn't yet have the requested block, immediately re_request block
			Request_Generator.force_re_request(conn->latest_request.front());
			conn->latest_request.pop_front();
			conn->wait_activated = true;
			conn->wait_start = time(NULL);
		}else if(block[0] == global::P_ERROR){
			logger::debug(LOGGER_P1,"server ",conn->IP," does not have file, REMOVAL FROM DB NOT IMPLEMENTED");
		}else{
			logger::debug(LOGGER_P1,"logic error: unhandled case");
			exit(1);
		}
		return;
	}else if(conn->State == connection_special::CLOSED_SLOT){
		//done with server, ignore any pending responses from it
		return;
	}

	logger::debug(LOGGER_P1,"logic error: unhandled case");
	exit(1);
}

const boost::uint64_t download_file::size()
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

	//cancelled downloads don't get added to share upon completion
	_cancel = true;

	//make invisible, cancelling download may take a while
	_visible = false;
}

void download_file::write_block(boost::uint64_t block_number, std::string & block)
{
	if(_cancel){
		return;
	}

	std::fstream fout(file_path.c_str(), std::ios::in | std::ios::out | std::ios::binary);
	if(fout.is_open()){
		fout.seekp(block_number * global::FILE_BLOCK_SIZE, std::ios::beg);
		fout.write(block.c_str(), block.size());
	}else{
		logger::debug(LOGGER_P1,"error opening file ",file_path);
		stop();
	}
}

void download_file::unregister_connection(const int & socket)
{
	//re_request all blocks that are pending for the server that's getting disconnected
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	if(iter != Connection_Special.end()){
		std::deque<boost::uint64_t>::iterator RB_iter_cur, RB_iter_end;
		RB_iter_cur = iter->second.latest_request.begin();
		RB_iter_end = iter->second.latest_request.end();
		while(RB_iter_cur != RB_iter_end){
			Request_Generator.force_re_request(*RB_iter_cur);
			++RB_iter_cur;
		}
	}

	download::unregister_connection(socket);
	Connection_Special.erase(socket);
}

bool download_file::visible()
{
	return _visible;
}
