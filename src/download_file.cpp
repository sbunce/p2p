#include "download_file.hpp"

boost::once_flag download_file::once_flag = BOOST_ONCE_INIT;
boost::mutex * download_file::_hashing_mutex;

download_file::download_file(
	const download_info & Download_Info_in
):
	Download_Info(Download_Info_in),
	Tree_Info(Download_Info_in.hash, Download_Info_in.size),
	hashing(true),
	hashing_percent(0),
	download_complete(false),
	close_slots(false)
{
	LOGGER << "ctor download_file: " << Download_Info.name;
	block_arbiter::singleton().start_file(Download_Info.hash, Tree_Info.get_file_block_count());
	visible = true;

	//create empty file for download if one doesn't already exist
	std::fstream fs(Download_Info.file_path.c_str(), std::ios::in | std::ios::ate);
	if(fs.is_open()){
		first_unreceived = fs.tellg() / global::FILE_BLOCK_SIZE;
	}else{
		fs.open(Download_Info.file_path.c_str(), std::ios::out);
		first_unreceived = 0;
	}

	Request_Generator =
		boost::shared_ptr<request_generator>(new request_generator((boost::uint64_t)first_unreceived,
		Tree_Info.get_file_block_count(), global::RE_REQUEST));

	bytes_received += (boost::uint64_t)first_unreceived * global::FILE_BLOCK_SIZE;

	if(first_unreceived == (boost::uint64_t)0){
		hashing = false;
	}else{
		hashing_thread = boost::thread(boost::bind(&download_file::hash_check, this, Tree_Info, Download_Info.file_path));
	}
}

download_file::~download_file()
{
	LOGGER << "dtor download_file: " << Download_Info.name;
	hashing_thread.interrupt();
	hashing_thread.join();
	if(cancel){
		std::remove(Download_Info.file_path.c_str());
	}
	block_arbiter::singleton().finish_download(Download_Info.hash);
}

bool download_file::complete()
{
	return download_complete;
}

download_info download_file::get_download_info()
{
	return Download_Info;
}

const std::string download_file::hash()
{
	return Download_Info.hash;
}

void download_file::hash_check(hash_tree::tree_info & Tree_Info, std::string file_path)
{
	boost::mutex::scoped_lock lock(hashing_mutex());

	//thread local buffer
	char block_buff[global::FILE_BLOCK_SIZE];

	std::fstream fin(file_path.c_str(), std::ios::in | std::ios::binary);
	assert(fin.good());
	boost::uint64_t check_block = 0;
	while(true){
		boost::this_thread::interruption_point();
		if(check_block == first_unreceived){
			break;
		}
		fin.read(block_buff, global::FILE_BLOCK_SIZE);
		assert(fin.good());
		hash_tree::status status = Hash_Tree.check_file_block(Tree_Info, check_block, block_buff, fin.gcount());
		if(status == hash_tree::GOOD){
			block_arbiter::singleton().add_file_block(Tree_Info.get_root_hash(), check_block);
		}else if(status == hash_tree::BAD){
			LOGGER << "found corrupt block " << check_block << " in resumed download";
			Request_Generator->force_rerequest(check_block);
		}else if(status == hash_tree::IO_ERROR){
			LOGGER << "IO_ERROR";
			exit(1);
		}else{
			LOGGER << "programmer error";
			exit(1);
		}
		++check_block;
		hashing_percent = (int)(((double)check_block / first_unreceived) * 100);
	}

	if(check_block == Tree_Info.get_file_block_count() && Request_Generator->complete()){
		close_slots = true;
	}

	hashing = false;
}

const std::string download_file::name()
{
	std::string temp = Download_Info.name;
	if(hashing){
		std::stringstream percent;
		percent << " CHECK " << hashing_percent << "%";
		temp += percent.str();
	}
	if(paused){
		temp += " PAUSED";
	}
	return temp;
}

void download_file::pause()
{
	LOGGER << "paused " << Download_Info.hash;
	paused = !paused;

	if(paused){
		LOGGER << "paused " << Download_Info.name;
	}else{
		LOGGER << "unpaused " << Download_Info.name;
	}

	std::map<int, connection_special>::iterator iter_cur, iter_end;
	iter_cur = Connection_Special.begin();
	iter_end = Connection_Special.end();
	while(iter_cur != iter_end){
		if(paused){
			iter_cur->second.State = connection_special::PAUSED;
		}else{
			if(!iter_cur->second.slot_open){
				iter_cur->second.State = connection_special::REQUEST_SLOT;
			}else{
				iter_cur->second.State = connection_special::REQUEST_BLOCKS;
			}
		}
		++iter_cur;
	}
}

unsigned download_file::percent_complete()
{
	if(Tree_Info.get_file_block_count() == 0){
		return 0;
	}else{
		unsigned percent = (unsigned)(((float)bytes_received / (float)Tree_Info.get_file_size())*100);
		if(percent > 100){
			percent = 100;
		}
		return percent;
	}
}

void download_file::protocol_block(std::string & message, connection_special * conn)
{
	if(!cancel){
		message.erase(0, 1); //trim command
		hash_tree::status status = Hash_Tree.check_file_block(Tree_Info,
			conn->latest_request.front(), message.c_str(), message.size());
		if(status == hash_tree::GOOD){
			write_block(conn->latest_request.front(), message);
			block_arbiter::singleton().add_file_block(Download_Info.hash, conn->latest_request.front());
			Request_Generator->fulfil(conn->latest_request.front());
		}else if(status == hash_tree::BAD){
			LOGGER << Download_Info.name << ":" << conn->latest_request.front() << " hash failure";
			bytes_received -= message.size() + 1; //correct bytes_recieved for percent calculation
			Request_Generator->force_rerequest(conn->latest_request.front());
			#ifndef CORRUPT_FILE_BLOCK_TEST
			database::table::blacklist::add(conn->IP, DB);
			#endif
		}else if(status == hash_tree::IO_ERROR){
			LOGGER << "IO_ERROR";
			paused = false;
			pause();
		}else{
			LOGGER << "programmer error";
			exit(1);
		}
	}
	conn->latest_request.pop_front();
	if(Request_Generator->complete() && !hashing){
		//download is complete, start closing slots
		close_slots = true;
	}
}

void download_file::protocol_slot(std::string & message, connection_special * conn)
{
	conn->slot_open = true;
	conn->slot_ID = message[1];
	conn->State = connection_special::REQUEST_BLOCKS;
}

void download_file::protocol_wait(std::string & message, connection_special * conn)
{
	//server doesn't have requested block, request from different server
	Request_Generator->force_rerequest(conn->latest_request.front());
	conn->latest_request.pop_front();

	//setup info needed for wait time
	conn->wait_activated = true;
	conn->wait_start = time(NULL);
}

void download_file::register_connection(const download_connection & DC)
{
	download::register_connection(DC);
	Connection_Special.insert(std::make_pair(DC.socket, connection_special(DC.IP)));
}

download::mode download_file::request(const int & socket, std::string & request,
	std::vector<std::pair<char, int> > & expected, int & slots_used)
{
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	assert(iter != Connection_Special.end());
	connection_special * conn = &iter->second;

	if(conn->State == connection_special::REQUEST_SLOT){
		if(close_slots || conn->slot_requested || slots_used >= 255){
			return download::NO_REQUEST;
		}else{
			conn->slot_requested = true;
			++slots_used;
			std::string hash_bin;
			if(!convert::hex_to_bin(Download_Info.hash, hash_bin)){
				LOGGER << "invalid hex";
				exit(1);
			}
			request = global::P_REQUEST_SLOT_FILE + hash_bin;
			expected.push_back(std::make_pair(global::P_SLOT, global::P_SLOT_SIZE));
			expected.push_back(std::make_pair(global::P_ERROR, global::P_ERROR_SIZE));
			return download::BINARY_MODE;
		}
	}else if(conn->State == connection_special::REQUEST_BLOCKS){
		if(download_complete){
			return download::NO_REQUEST;
		}else if(close_slots){
			if(!conn->slot_open){
				 return download::NO_REQUEST;
			}else{
				conn->slot_requested = false;
				conn->slot_open = false;
				request += global::P_CLOSE_SLOT;
				request += conn->slot_ID;
				--slots_used;

				//download complete unless slot open with a server
				download_complete = true;
				std::map<int, connection_special>::iterator iter_cur, iter_end;
				iter_cur = Connection_Special.begin();
				iter_end = Connection_Special.end();
				bool unready_found = false;
				while(iter_cur != iter_end){
					if(iter_cur->second.slot_open){
						download_complete = false;
						break;
					}
					++iter_cur;
				}
				return download::BINARY_MODE;
			}
		}else{
			if(conn->wait_activated){
				if(conn->wait_start + global::P_WAIT_TIMEOUT <= time(NULL)){
					conn->wait_activated = false;
				}else{
					return download::NO_REQUEST;
				}
			}

			if(!Request_Generator->complete() && Request_Generator->request(conn->latest_request)){
				request += global::P_REQUEST_BLOCK;
				request += conn->slot_ID;
				request += convert::encode<boost::uint64_t>(conn->latest_request.back());
				int size;
				if(conn->latest_request.back() == Tree_Info.get_file_block_count() - 1){
					size = Tree_Info.get_last_file_block_size() + 1; //+1 for command size
					Request_Generator->set_timeout(global::RE_REQUEST_FINISHING);
				}else{
					size = global::P_BLOCK_SIZE;
				}
				expected.push_back(std::make_pair(global::P_BLOCK, size));
				expected.push_back(std::make_pair(global::P_ERROR, global::P_ERROR_SIZE));
				expected.push_back(std::make_pair(global::P_WAIT, global::P_WAIT_SIZE));
				return download::BINARY_MODE;
			}else{
				return download::NO_REQUEST;
			}
		}
	}else if(conn->State == connection_special::PAUSED){
		if(conn->slot_open){
			conn->slot_requested = false;
			conn->slot_open = false;
			--slots_used;
			request += global::P_CLOSE_SLOT;
			request += conn->slot_ID;
			return download::BINARY_MODE;
		}else{
			return download::NO_REQUEST;
		}
	}else{
		LOGGER << "programmer error";
		exit(1);
	}
}

bool download_file::response(const int & socket, std::string message)
{
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	assert(iter != Connection_Special.end());
	connection_special * conn = &iter->second;

	if(message[0] == global::P_SLOT){
		protocol_slot(message, conn);
	}else if(message[0] == global::P_ERROR){
		LOGGER << conn->IP << " doesn't have hash tree, REMOVAL FROM DB NOT IMPLEMENTED";
		return false;
	}else if(message[0] == global::P_BLOCK){
		protocol_block(message, conn);
	}else if(message[0] == global::P_WAIT){
		protocol_wait(message, conn);
	}else{
		LOGGER << "programmer error";
		exit(1);
	}
	return true;
}

const boost::uint64_t download_file::size()
{
	return Download_Info.size;
}

void download_file::remove()
{
	if(download::connection_count() == 0){
		download_complete = true;
	}else{
		close_slots = true;
	}

	//cancelled downloads don't get added to share upon completion
	cancel = true;

	//make invisible, cancelling download may take a while
	visible = false;

	database::table::download::terminate(Download_Info.hash, Download_Info.size, DB);
}

void download_file::write_block(boost::uint64_t block_number, std::string & block)
{
	if(cancel){
		return;
	}

	std::fstream fout(Download_Info.file_path.c_str(), std::ios::in | std::ios::out | std::ios::binary);
	if(fout.is_open()){
		fout.seekp(block_number * global::FILE_BLOCK_SIZE, std::ios::beg);
		fout.write(block.c_str(), block.size());
	}else{
		LOGGER << "error opening file " << Download_Info.file_path;
		paused = false;
		pause();
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
			Request_Generator->force_rerequest(*RB_iter_cur);
			++RB_iter_cur;
		}
	}

	download::unregister_connection(socket);
	Connection_Special.erase(socket);
}
