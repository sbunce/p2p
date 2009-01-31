#include "download_hash_tree.hpp"

download_hash_tree::download_hash_tree(
	const download_info & Download_Info_in
):
	Download_Info(Download_Info_in),
	download_complete(false),
	close_slots(false),
	_cancel(false),
	_visible(true),
	Tree_Info(Download_Info_in.hash, Download_Info_in.size),
	DB_Hash(DB)
{
	client_server_bridge::start_hash_tree(Download_Info.hash);
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
	if(_cancel){
		DB_Download.terminate(Download_Info.hash, Download_Info.size);
		client_server_bridge::finish_download(Download_Info.hash);
		std::remove((global::DOWNLOAD_DIRECTORY + Download_Info.name).c_str());
	}else{
		DB_Hash.set_state(Download_Info.hash, Tree_Info.get_tree_size(), database::table::hash::COMPLETE);
	}
}

const bool & download_hash_tree::canceled()
{
	return _cancel;
}

bool download_hash_tree::complete()
{
	return download_complete;
}

const download_info & download_hash_tree::get_download_info()
{
	return Download_Info;
}

const std::string download_hash_tree::hash()
{
	return Download_Info.hash;
}

const std::string download_hash_tree::name()
{
	return Download_Info.name + " HASH";
}

unsigned download_hash_tree::percent_complete()
{
	if(Tree_Info.get_block_count() == 0){
		return 0;
	}else{
		return (unsigned)(((float)Request_Generator.highest_requested() / (float)Tree_Info.get_block_count())*100);
	}
}

download::mode download_hash_tree::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected, int & slots_used)
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
			//no free slots left, wait for one to free up
			return download::NO_REQUEST;
		}else{
			request = global::P_REQUEST_SLOT_HASH + convert::hex_to_bin(Download_Info.hash);
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

	LOGGER << "logic error: unhandled case";
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

	if(conn->State == connection_special::AWAITING_SLOT){
		if(block[0] == global::P_SLOT_ID){
			//received slot, ready to request blocks
			conn->slot_ID = block[1];
			conn->State = connection_special::REQUEST_BLOCKS;
		}else if(block[0] == global::P_ERROR){
			LOGGER << "server " << conn->IP << " does not have hash tree, REMOVAL FROM DB NOT IMPLEMENTED";
		}else{
			LOGGER << "logic error: unhandled case";
			exit(1);
		}
		return;
	}else if(conn->State == connection_special::REQUEST_BLOCKS){
		if(block[0] == global::P_BLOCK){
			//a block was received
			block.erase(0, 1); //trim command

			//write hash only if not cancelled
			if(!_cancel && !Hash_Tree.write_block(Tree_Info, conn->latest_request.front(), block, conn->IP)){
				//hash file removed or download cancelled, stop download
				stop();
			}

			boost::uint64_t highest_good;
			if(Tree_Info.highest_good(highest_good)){
				client_server_bridge::update_hash_tree_highest(Download_Info.hash, highest_good);
			}

			Request_Generator.fulfil(conn->latest_request.front());
			conn->latest_request.pop_front();

			//see if hash tree has any blocks to rerequest
			Tree_Info.rerequest_bad_blocks(Request_Generator);

			if(Request_Generator.complete()){
				//hash tree complete, close slots
				close_slots = true;
			}
		}else if(block[0] == global::P_WAIT){
			//server doesn't yet have the requested block, immediately re_request block
			Request_Generator.force_rerequest(conn->latest_request.front());
			conn->latest_request.pop_front();
			conn->wait_activated = true;
			conn->wait_start = time(NULL);
		}else if(block[0] == global::P_ERROR){
			LOGGER << "server " << conn->IP << " does not have hash tree, REMOVAL FROM DB NOT IMPLEMENTED";
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
		std::deque<boost::uint64_t>::iterator iter_cur, iter_end;
		iter_cur = iter->second.latest_request.begin();
		iter_end = iter->second.latest_request.end();
		while(iter_cur != iter_end){
			Request_Generator.force_rerequest(*iter_cur);
			++iter_cur;
		}
	}

	download::unregister_connection(socket);
	Connection_Special.erase(socket);
}

bool download_hash_tree::visible()
{
	return _visible;
}
