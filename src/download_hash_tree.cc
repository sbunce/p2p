/*
make a download_hash_tree_conn

it should contain a vector of blocks requested.

if a block fails from a server all blocks that were downloaded it should be
force_re_request'ed from other servers and the server disconnected.

*/

#include "download_hash_tree.h"

download_hash_tree::download_hash_tree(
	const std::string & root_hash_in,
	const uint64_t & download_file_size_in,
	const std::string & download_file_name_in
):
	root_hash(root_hash_in),
	_download_file_size(download_file_size_in),
	_download_file_name(download_file_name_in),
	download_complete(false),
	close_slots(false),
	_canceled(false),
	checking_phase(false)
{
	assert(global::FILE_BLOCK_SIZE % sha::HASH_LENGTH == 0);
	hash_name = _download_file_name + " HASH";
	hash_tree_count = hash_tree::hash_tree_count(hash_tree::file_hash_count(_download_file_size));
	hashes_per_block = global::FILE_BLOCK_SIZE / sha::HASH_LENGTH;
	hash_block_count = (hash_tree_count * sha::HASH_LENGTH) / global::FILE_BLOCK_SIZE;
	if((hash_tree_count * sha::HASH_LENGTH) % global::FILE_BLOCK_SIZE != 0){
		++hash_block_count;
	}

	/*
	Create empty file for the hash tree, if it doesn't already exist.
	*/
	bool hash_tree_exists;
	std::fstream fin((global::HASH_DIRECTORY+root_hash).c_str(), std::ios::in);
	if(fin.is_open()){
		hash_tree_exists = true;
		fin.close();
	}else{
		hash_tree_exists = false;
		std::fstream fout((global::HASH_DIRECTORY+root_hash).c_str(), std::ios::out);
		fout.write(convert::hex_to_binary(root_hash).c_str(), sha::HASH_LENGTH);
		fout.close();
	}

	/*
	Create an empty file for the file the hash tree is for. This is needed so the
	download isn't detected as removed when the program is stopped/started. Removed
	downloads are cancelled which this prevents.
	*/
	fin.open((global::DOWNLOAD_DIRECTORY+_download_file_name).c_str(), std::ios::in);
	if(!fin.is_open()){
		std::fstream fout((global::DOWNLOAD_DIRECTORY+_download_file_name).c_str(), std::ios::out);
		fout.close();
	}

	/*
	Check the hash tree to see if it's complete/corrupt/missing blocks. Resume
	downloading the hash tree or mark the download as complete if the hash tree
	is complete.
	*/
	if(hash_tree_exists){
		std::pair<uint64_t, uint64_t> bad_hash;
		if(Hash_Tree.check_hash_tree(root_hash, hash_tree_count, bad_hash)){
			//determine what hash_block the missing/bad hash falls within
			uint64_t start_hash_block = bad_hash.first / global::FILE_BLOCK_SIZE;
			logger::debug(LOGGER_P1,"partial or corrupt hash tree found, resuming on hash block ",start_hash_block);
			Request_Gen.init(start_hash_block, hash_block_count - 1, global::RE_REQUEST_TIMEOUT);
		}else{
			//complete hash tree
			download_complete = true;
		}
	}else{
		Request_Gen.init(0, hash_block_count - 1, global::RE_REQUEST_TIMEOUT);
	}
}

const bool & download_hash_tree::canceled()
{
	return _canceled;
}

bool download_hash_tree::complete()
{
	return download_complete;
}

const uint64_t & download_hash_tree::download_file_size()
{
	return _download_file_size;
}

const std::string & download_hash_tree::download_file_name()
{
	return _download_file_name;
}

const std::string download_hash_tree::hash()
{
	return root_hash;
}

const std::string download_hash_tree::name()
{
	return hash_name;
}

unsigned int download_hash_tree::percent_complete()
{
	if(hash_block_count == 0){
		return 0;
	}else{
		return (unsigned int)(((float)Request_Gen.highest_requested() / (float)hash_block_count)*100);
	}
}

download::mode download_hash_tree::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected)
{
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	assert(iter != Connection_Special.end());
	connection_special * conn = &iter->second;

	if(conn->close_slot_sent || conn->abusive){
		return download::NO_REQUEST;
	}

	if(!conn->slot_ID_requested){
		//slot_ID not yet obtained from server
		request = global::P_REQUEST_SLOT_HASH + convert::hex_to_binary(root_hash);
		conn->slot_ID_requested = true;
		expected.push_back(std::make_pair(global::P_SLOT_ID, global::P_SLOT_ID_SIZE));
		expected.push_back(std::make_pair(global::P_ERROR, 1));
		return download::BINARY_MODE;
	}else if(!conn->slot_ID_received){
		//slot_ID requested but not yet received
		return download::NO_REQUEST;
	}

	if(close_slots && !conn->close_slot_sent){
		//download finishing or cancelled, send P_CLOSE_SLOT
		request += global::P_CLOSE_SLOT;
		request += conn->slot_ID;
		conn->close_slot_sent = true;

		//the download is complete when all servers have been sent a P_CLOSE_SLOT
		bool unready_found = false;
		std::map<int, connection_special>::iterator CS_iter_cur, CS_iter_end;
		CS_iter_cur = Connection_Special.begin();
		CS_iter_end = Connection_Special.end();
		while(CS_iter_cur != CS_iter_end){
			if(CS_iter_cur->second.close_slot_sent == false){
				unready_found = true;
			}
			++CS_iter_cur;
		}
		if(!unready_found){
			download_complete = true;
		}
		return download::BINARY_MODE;
	}

	if(download_complete){
		return download::NO_REQUEST;
	}

	if(Request_Gen.request(conn->latest_request)){
		//no request to be made at the moment
		request += global::P_SEND_BLOCK;
		request += conn->slot_ID;
		request += convert::encode<uint64_t>(conn->latest_request.back());
		conn->requested_blocks.insert(conn->latest_request.back());
		int size;
		if(hash_tree_count - (conn->latest_request.back() * hashes_per_block) < hashes_per_block){
			//request will not yield full P_BLOCK
			size = (hash_tree_count - (conn->latest_request.back() * hashes_per_block)) * sha::HASH_LENGTH + 1;
		}else{
			size = global::P_BLOCK_SIZE;
		}
		expected.push_back(std::make_pair(global::P_BLOCK, size));
		expected.push_back(std::make_pair(global::P_ERROR, 1));
		return download::BINARY_MODE;
	}

	return download::NO_REQUEST;
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

	if(conn->abusive || close_slots){
		//server abusive but not yet disconnected, ignore data from it
		return;
	}

	if(block[0] == global::P_SLOT_ID && conn->slot_ID_received == false){
		conn->slot_ID = block[1];
		conn->slot_ID_received = true;
	}else if(block[0] == global::P_BLOCK){
		//a block was received
		Speed_Calculator.update(block.size()); //update download speed
		block.erase(0, 1);                     //trim command
		Hash_Tree.write_hash(root_hash, conn->latest_request.front()*hashes_per_block, block);
		Request_Gen.fulfil(conn->latest_request.front());
		conn->latest_request.pop_front();

		if(Request_Gen.complete()){
			//check for bad blocks in tree
			std::pair<uint64_t, uint64_t> bad_hash;
			if(Hash_Tree.check_hash_tree(root_hash, hash_tree_count, bad_hash)){
				//bad block found, find server that sent it
				uint64_t bad_block = bad_hash.first / hashes_per_block;
				std::map<int, connection_special>::iterator CS_iter_cur, CS_iter_end;
				CS_iter_cur = Connection_Special.begin();
				CS_iter_end = Connection_Special.end();
				while(CS_iter_cur != CS_iter_end){
					std::set<uint64_t>::iterator iter = CS_iter_cur->second.requested_blocks.find(bad_block);
					if(iter != CS_iter_cur->second.requested_blocks.end()){
						//re_request all blocks gotten from the server that sent a bad block
						CS_iter_cur->second.abusive = true;
						DB_blacklist::add(CS_iter_cur->second.IP);
						break;
					}
					++CS_iter_cur;
				}
			}else{
				close_slots = true;
			}
		}
	}else if(block[0] == global::P_ERROR){
		logger::debug(LOGGER_P1,"P_ERROR received from ",conn->IP);
	}
}

void download_hash_tree::stop()
{
	_canceled = true;
	if(Connection.size() == 0){
		download_complete = true;
	}else{
		close_slots = true;
	}
}

const uint64_t download_hash_tree::size()
{
	return hash_tree_count * sha::HASH_LENGTH;
}

void download_hash_tree::unregister_connection(const int & socket)
{
	//re_request all blocks that are pending for the server that's getting disconnected
	std::map<int, connection_special>::iterator iter = Connection_Special.find(socket);
	if(iter != Connection_Special.end()){
		std::set<uint64_t>::iterator RB_iter_cur, RB_iter_end;
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

bool download_hash_tree::visible()
{
	return true;
}
