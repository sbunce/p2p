#include "download_hash_tree.h"

download_hash_tree::download_hash_tree(
	const std::string & root_hash_hex_in,
	const boost::uint64_t & download_file_size_in,
	const std::string & download_file_name_in
):
	root_hash_hex(root_hash_hex_in),
	_download_file_size(download_file_size_in),
	_download_file_name(download_file_name_in),
	download_complete(false),
	close_slots(false),
	_cancel(false),
	_visible(true),
	Tree_Info(root_hash_hex, _download_file_size)
{
	client_server_bridge::start_download(root_hash_hex);
	hash_name = _download_file_name + " HASH";

	//create empty file for hash tree (hash_tree expects this)
	std::fstream fin((global::HASH_DIRECTORY+root_hash_hex).c_str(), std::ios::in);
	if(!fin.is_open()){
		//hash tree does not exist yet
		fin.open((global::HASH_DIRECTORY+root_hash_hex).c_str(), std::ios::out);
	}

	boost::uint64_t bad_block;
	if(Hash_Tree.check(Tree_Info, bad_block)){
		//hash tree good, signal download_complete
		download_complete = true;
	}else{
		//bad hash block detected
		Request_Generator.init(bad_block, Tree_Info.get_block_count(), global::RE_REQUEST);
	}
}

download_hash_tree::~download_hash_tree()
{

}

const bool & download_hash_tree::canceled()
{
	return _cancel;
}

bool download_hash_tree::complete()
{
	return download_complete;
}

const boost::uint64_t & download_hash_tree::download_file_size()
{
	return _download_file_size;
}

const std::string & download_hash_tree::download_file_name()
{
	return _download_file_name;
}

const std::string download_hash_tree::hash()
{
	return root_hash_hex;
}

const std::string download_hash_tree::name()
{
	return hash_name;
}

unsigned int download_hash_tree::percent_complete()
{
	if(Tree_Info.get_block_count() == 0){
		return 0;
	}else{
		return (unsigned int)(((float)Request_Generator.highest_requested() / (float)Tree_Info.get_block_count())*100);
	}
}

download::mode download_hash_tree::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected, int & slots_used)
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
			//no free slots left, wait for one to free up
			return download::NO_REQUEST;
		}else{
			request = global::P_REQUEST_SLOT_HASH + convert::hex_to_binary(root_hash_hex);
			++slots_used;
			expected.push_back(std::make_pair(global::P_SLOT_ID, global::P_SLOT_ID_SIZE));
			expected.push_back(std::make_pair(global::P_ERROR, global::P_ERROR_SIZE));
			conn->State = connection_special::AWAITING_SLOT;
			return download::BINARY_MODE;
		}
	}else if(conn->State == connection_special::AWAITING_SLOT){
		//wait until slot_ID is received
		return download::NO_REQUEST;
	}else if(conn->State == connection_special::REQUEST_BLOCKS){
		if(close_slots){
			//download finishing or cancelled, send P_CLOSE_SLOT
			request += global::P_CLOSE_SLOT;
			request += conn->slot_ID;
			--slots_used;
			conn->State = connection_special::CLOSED_SLOT;

			//check if all servers have been sent a P_CLOSE_SLOT
			std::map<int, connection_special>::iterator iter_cur, iter_end;
			iter_cur = Connection_Special.begin();
			iter_end = Connection_Special.end();
			bool unready_found = false;
			while(iter_cur != iter_end){
				if(iter_cur->second.State != connection_special::CLOSED_SLOT){
					//server requested slot but has not yet closed it
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

		if(Request_Generator.request(conn->latest_request)){
			//no request to be made at the moment
			request += global::P_BLOCK;
			request += conn->slot_ID;
			request += convert::encode<boost::uint64_t>(conn->latest_request.back());
			conn->requested_blocks.insert(conn->latest_request.back());
			expected.push_back(std::make_pair(global::P_BLOCK, hash_tree::block_size(Tree_Info, conn->latest_request.back()) + 1));
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

void download_hash_tree::register_connection(const download_connection & DC)
{
	download::register_connection(DC);
	Connection_Special.insert(std::make_pair(DC.socket, connection_special(DC.IP)));
}

void download_hash_tree::response(const int & socket, std::string block)
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
			logger::debug(LOGGER_P1,"server ",conn->IP," does not have hash tree, REMOVAL FROM DB NOT IMPLEMENTED");
		}else{
			logger::debug(LOGGER_P1,"logic error: unhandled case");
			exit(1);
		}
		return;
	}else if(conn->State == connection_special::REQUEST_BLOCKS){
		if(block[0] == global::P_BLOCK){
			//a block was received
			block.erase(0, 1); //trim command

			//write hash only if not cancelled
			if(!_cancel && !Hash_Tree.write_block(Tree_Info, conn->latest_request.front(), block)){
				//hash file removed, stop download
				stop();
			}

			client_server_bridge::download_block_received(root_hash_hex, conn->latest_request.front());
			Request_Generator.fulfil(conn->latest_request.front());
			conn->latest_request.pop_front();
			if(Request_Generator.complete()){
				//check for bad blocks in tree
				boost::uint64_t bad_block;
				if(Hash_Tree.check(Tree_Info, bad_block)){
					//hash tree complete, close slots
					close_slots = true;
				}else{
					//bad block found, find server that sent it
					std::map<int, connection_special>::iterator iter_cur, iter_end;
					iter_cur = Connection_Special.begin();
					iter_end = Connection_Special.end();
					while(iter_cur != iter_end){
						std::set<boost::uint64_t>::iterator iter = iter_cur->second.requested_blocks.find(bad_block);
						if(iter != iter_cur->second.requested_blocks.end()){
							//re_request all blocks gotten from the server that sent a bad block
							iter_cur->second.State = connection_special::ABUSIVE;
							DB_blacklist::add(iter_cur->second.IP);
							return;
						}
						++iter_cur;
					}
				}
			}
		}else if(block[0] == global::P_WAIT){
			//server doesn't yet have the requested block, immediately re_request block
			Request_Generator.force_re_request(conn->latest_request.front());
			conn->latest_request.pop_front();
			conn->wait_activated = true;
			conn->wait_start = time(NULL);
		}else if(block[0] == global::P_ERROR){
			logger::debug(LOGGER_P1,"server ",conn->IP," does not have hash tree, REMOVAL FROM DB NOT IMPLEMENTED");
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

void download_hash_tree::stop()
{
	_cancel = true;
	_visible = false;
	if(Connection.size() == 0){
		download_complete = true;
	}else{
		close_slots = true;
	}
}

const boost::uint64_t download_hash_tree::size()
{
	return Tree_Info.get_tree_size();
}

void download_hash_tree::unregister_connection(const int & socket)
{
	//re_request all blocks that are pending for the server that's getting disconnected
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	if(iter != Connection_Special.end()){
		std::set<boost::uint64_t>::iterator RB_iter_cur, RB_iter_end;
		RB_iter_cur = iter->second.requested_blocks.begin();
		RB_iter_end = iter->second.requested_blocks.end();
		while(RB_iter_cur != RB_iter_end){
			Request_Generator.force_re_request(*RB_iter_cur);
			++RB_iter_cur;
		}
	}

	download::unregister_connection(socket);
	Connection_Special.erase(socket);
}

bool download_hash_tree::visible()
{
	return _visible;
}
