#include "download_hash_tree.hpp"

download_hash_tree::download_hash_tree(
	const download_info & Download_Info_in
):
	Download_Info(Download_Info_in),
	download_complete(false),
	close_slots(false),
	Tree_Info(Download_Info_in.hash, Download_Info_in.size)
{
	block_arbiter::start_hash_tree(Download_Info.hash);
	visible = true;
	boost::uint64_t bad_block;
	hash_tree::status status = Hash_Tree.check(Tree_Info, bad_block);
	if(status == hash_tree::GOOD){
		//hash tree good, signal download_complete
		download_complete = true;
	}else if(status == hash_tree::BAD){
		//bad hash block detected
		Request_Generator =
			boost::shared_ptr<request_generator>(new request_generator(bad_block,
			Tree_Info.get_block_count(), global::RE_REQUEST));

		for(boost::uint64_t x=0; x<bad_block; ++x){
			bytes_received += Tree_Info.block_size(x);
		}
	}else if(status == hash_tree::IO_ERROR){
		pause();
	}else{
		LOGGER << "programmer error";
		exit(1);
	}
}

download_hash_tree::~download_hash_tree()
{
	if(cancel){
		block_arbiter::finish_download(Download_Info.hash);
		std::remove((global::DOWNLOAD_DIRECTORY + Download_Info.name).c_str());
	}else{
		if(download_complete){
			database::table::hash::set_state(Download_Info.hash, Tree_Info.get_tree_size(), database::table::hash::COMPLETE, DB);
		}
	}
}

bool download_hash_tree::complete()
{
	return download_complete;
}

download_info download_hash_tree::get_download_info()
{
	return Download_Info;
}

const std::string download_hash_tree::hash()
{
	return Download_Info.hash;
}

const std::string download_hash_tree::name()
{
	if(paused){
		return Download_Info.name + " HASH PAUSED";
	}else{
		return Download_Info.name + " HASH";
	}
}

void download_hash_tree::pause()
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

unsigned download_hash_tree::percent_complete()
{
	if(Tree_Info.get_block_count() == 0){
		return 0;
	}else{
		unsigned percent = (unsigned)(((float)bytes_received / (float)Tree_Info.get_tree_size())*100);
		if(percent > 100){
			percent = 100;
		}
		return percent;
	}
}

void download_hash_tree::protocol_block(std::string & message, connection_special * conn)
{
	message.erase(0, 1); //trim command
	if(!cancel){
		hash_tree::status status = Hash_Tree.write_block(Tree_Info,
			conn->latest_request.front(), message, conn->IP);
		if(status == hash_tree::IO_ERROR){
			LOGGER << "IO_ERROR writing hash block, pausing download";
			paused = false;
			pause();
		}
	}

	boost::uint64_t highest_good;
	if(Tree_Info.highest_good(highest_good)){
		block_arbiter::update_hash_tree_highest(Download_Info.hash, highest_good);
	}

	Request_Generator->fulfil(conn->latest_request.front());
	conn->latest_request.pop_front();

	/*
	See if hash tree has any blocks to rerequest. If it does subtract the
	size of the blocks from bytes_received for the purpose of accurate
	percentage calculation.
	*/
	bytes_received -= Tree_Info.rerequest_bad_blocks(*Request_Generator);

	if(Request_Generator->complete()){
		//hash tree complete, close slots
		close_slots = true;
	}
}

void download_hash_tree::protocol_slot(std::string & message, connection_special * conn)
{
	conn->slot_open = true;
	conn->slot_ID = message[1];
	conn->State = connection_special::REQUEST_BLOCKS;
}

void download_hash_tree::protocol_wait(std::string & message, connection_special * conn)
{
	//server doesn't have requested block, request from different server
	Request_Generator->force_rerequest(conn->latest_request.front());
	conn->latest_request.pop_front();

	//setup info needed for wait time
	conn->wait_activated = true;
	conn->wait_start = time(NULL);
}

download::mode download_hash_tree::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected, int & slots_used)
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
			request = global::P_REQUEST_SLOT_HASH_TREE + convert::hex_to_bin(Download_Info.hash);
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
				--slots_used;
				request += global::P_CLOSE_SLOT;
				request += conn->slot_ID;

				//download complete unless slot open with a server
				download_complete = true;
				std::map<int, connection_special>::iterator iter_cur, iter_end;
				iter_cur = Connection_Special.begin();
				iter_end = Connection_Special.end();
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

			if(Request_Generator->request(conn->latest_request)){
				request += global::P_REQUEST_BLOCK;
				request += conn->slot_ID;
				request += convert::encode<boost::uint64_t>(conn->latest_request.back());
				expected.push_back(std::make_pair(global::P_BLOCK, Tree_Info.block_size(conn->latest_request.back()) + 1));
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

void download_hash_tree::register_connection(const download_connection & DC)
{
	download::register_connection(DC);
	Connection_Special.insert(std::make_pair(DC.socket, connection_special(DC.IP)));
}

bool download_hash_tree::response(const int & socket, std::string message)
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

const boost::uint64_t download_hash_tree::size()
{
	return Tree_Info.get_tree_size();
}

void download_hash_tree::remove()
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

void download_hash_tree::unregister_connection(const int & socket)
{
	//re_request all blocks that are pending for the server that's getting disconnected
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	if(iter != Connection_Special.end()){
		std::deque<boost::uint64_t>::iterator iter_cur, iter_end;
		iter_cur = iter->second.latest_request.begin();
		iter_end = iter->second.latest_request.end();
		while(iter_cur != iter_end){
			Request_Generator->force_rerequest(*iter_cur);
			++iter_cur;
		}
	}

	download::unregister_connection(socket);
	Connection_Special.erase(socket);
}
