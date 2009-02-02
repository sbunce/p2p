#include "download_file.hpp"

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
	client_server_bridge::start_file(Download_Info.hash, Tree_Info.get_file_block_count());

	visible = true;

	//create empty file for download if one doesn't already exist
	std::fstream fs(Download_Info.file_path.c_str(), std::ios::in | std::ios::ate);
	if(fs.is_open()){
		first_unreceived = fs.tellg() / global::FILE_BLOCK_SIZE;
	}else{
		fs.open(Download_Info.file_path.c_str(), std::ios::out);
		first_unreceived = 0;
	}

	Request_Generator.init((boost::uint64_t)first_unreceived, Tree_Info.get_file_block_count(), global::RE_REQUEST);

	//hash check for corrupt/missing blocks
	if(first_unreceived == 0){
		hashing = false;
	}else{
		hashing_thread = boost::thread(boost::bind(&download_file::hash_check, this, Tree_Info, Download_Info.file_path));
	}
}

download_file::~download_file()
{
	hashing_thread.interrupt();
	hashing_thread.join();
	if(cancel){
		std::remove(Download_Info.file_path.c_str());
	}
	client_server_bridge::finish_download(Download_Info.hash);
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

void download_file::hash_check(hash_tree::tree_info Tree_Info, std::string file_path)
{
	//only allow one hash_check job at a time for all downloads
	static int job_cnt = 0;
	static boost::mutex job_cnt_mutex;
	while(true){
		{
		boost::mutex::scoped_lock lock(job_cnt_mutex);
		if(job_cnt == 0){
			++job_cnt;
			break;
		}
		}
		portable_sleep::ms(1000);
	}

	//thread local stuff
	hash_tree Hash_Tree;
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
		if(Hash_Tree.check_file_block(Tree_Info, check_block, block_buff, fin.gcount())){
			client_server_bridge::add_file_block(Tree_Info.get_root_hash(), check_block);
		}else{
			LOGGER << "found corrupt block " << check_block << " in resumed download";
			Request_Generator.force_rerequest(check_block);
		}
		++check_block;
		hashing_percent = (int)(((double)check_block / first_unreceived) * 100);
	}
	hashing = false;

	{
	boost::mutex::scoped_lock lock(job_cnt_mutex);
	--job_cnt;
	}
}

const std::string download_file::name()
{
	if(hashing){
		std::stringstream name;
		name << Download_Info.name + " CHECK " << hashing_percent << "%";
		return name.str();
	}else{
		return Download_Info.name;
	}
}

unsigned download_file::percent_complete()
{
	if(Tree_Info.get_file_block_count() == 0){
		return 0;
	}else{
		return (unsigned)(((float)Request_Generator.highest_requested()
			/ (float)Tree_Info.get_file_block_count())*100);
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

	if(conn->State == connection_special::REQUEST_SLOT){
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
		request = global::P_REQUEST_SLOT_FILE + convert::hex_to_bin(Download_Info.hash);
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
			if(conn->latest_request.back() == Tree_Info.get_file_block_count() - 1){
				size = Tree_Info.get_last_file_block_size() + 1; //+1 for command
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

	LOGGER << "logic error: unhandled case";
	exit(1);
}

void download_file::response(const int & socket, std::string block)
{
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	assert(iter != Connection_Special.end());
	connection_special * conn = &iter->second;

	if(conn->State == connection_special::AWAITING_SLOT){
		if(block[0] == global::P_SLOT_ID){
			//received slot, ready to request blocks
			conn->slot_ID = block[1];
			conn->State = connection_special::REQUEST_BLOCKS;
		}else if(block[0] == global::P_ERROR){
			LOGGER << "server " << conn->IP << " does not have file, REMOVAL FROM DB NOT IMPLEMENTED";
		}else{
			LOGGER << "logic error: unhandled case";
			exit(1);
		}
		return;
	}else if(conn->State == connection_special::REQUEST_BLOCKS){
		if(block[0] == global::P_BLOCK){
			block.erase(0, 1); //trim command
			if(Hash_Tree.check_file_block(Tree_Info, conn->latest_request.front(), block.c_str(), block.length())){
				write_block(conn->latest_request.front(), block);
				client_server_bridge::add_file_block(Download_Info.hash, conn->latest_request.front());
				Request_Generator.fulfil(conn->latest_request.front());
			}else{
				LOGGER << Download_Info.name << ":" << conn->latest_request.front() << " hash failure";
				Request_Generator.force_rerequest(conn->latest_request.front());
				#ifndef CORRUPT_FILE_BLOCK_TEST
				DB_Blacklist.add(conn->IP);
				#endif
			}
			conn->latest_request.pop_front();
			if(Request_Generator.complete() && !hashing){
				//download is complete, start closing slots
				close_slots = true;
			}
		}else if(block[0] == global::P_WAIT){
			//server doesn't yet have the requested block, immediately re_request block
			Request_Generator.force_rerequest(conn->latest_request.front());
			conn->latest_request.pop_front();
			conn->wait_activated = true;
			conn->wait_start = time(NULL);
		}else if(block[0] == global::P_ERROR){
			LOGGER << "server " << conn->IP << " does not have file, REMOVAL FROM DB NOT IMPLEMENTED";
		}else{
			LOGGER << "logic error: unhandled case";
			exit(1);
		}
		return;
	}else if(conn->State == connection_special::CLOSED_SLOT){
		//done with server, ignore any pending responses from it
		return;
	}

	LOGGER << "logic error: unhandled case";
	exit(1);
}

const boost::uint64_t download_file::size()
{
	return Download_Info.size;
}

void download_file::stop()
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
			Request_Generator.force_rerequest(*RB_iter_cur);
			++RB_iter_cur;
		}
	}

	download::unregister_connection(socket);
	Connection_Special.erase(socket);
}
