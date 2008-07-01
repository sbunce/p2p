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
	std::fstream fin((global::HASH_DIRECTORY+root_hash).c_str(), std::ios::in);
	if(fin.is_open()){
		fin.close();
	}else{
		std::fstream fout((global::HASH_DIRECTORY+root_hash).c_str(), std::ios::out);
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
	std::pair<uint64_t, uint64_t> bad_hash;
	if(Hash_Tree.check_hash_tree(root_hash, hash_tree_count, bad_hash)){
		//determine what hash_block the missing/bad hash falls within
		uint64_t start_hash_block = bad_hash.first / global::FILE_BLOCK_SIZE;
		logger::debug(LOGGER_P1,"partial or corrupt hash tree found, resuming on hash block ",start_hash_block);
		Request_Gen.init(start_hash_block, hash_block_count - 1, global::RE_REQUEST_TIMEOUT);
	}else{
		//complete hash tree
		download_complete = true;
		Request_Gen.init(0, hash_block_count - 1, global::RE_REQUEST_TIMEOUT);
	}
}

bool download_hash_tree::complete()
{
	return download_complete;
}

const std::string & download_hash_tree::hash()
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

bool download_hash_tree::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected)
{
	std::map<int, download_conn *>::iterator iter = Connection.find(socket);
	assert(iter != Connection.end());
	download_hash_tree_conn * conn = dynamic_cast<download_hash_tree_conn *>(iter->second);
	assert(conn != NULL);

	if(Request_Gen.complete()){
		checking_phase = true;
		/*
		All blocks have been requested and received, check the hash tree for bad
		hash blocks.
		*/
		std::pair<uint64_t, uint64_t> bad_hash;
		if(Hash_Tree.check_hash_tree(root_hash, hash_tree_count, bad_hash)){
			/*
			There is a bad hash block. Find out what server it was from and re_request
			all blocks that were requested from that server.
			*/
			uint64_t bad_block = bad_hash.first / hashes_per_block;

			std::map<int, download_conn *>::iterator C_iter_cur, C_iter_end;
			C_iter_cur = Connection.begin();
			C_iter_end = Connection.end();
			download_hash_tree_conn * DHTC;
			while(C_iter_cur != C_iter_end){
				DHTC = dynamic_cast<download_hash_tree_conn *>(C_iter_cur->second);
				assert(DHTC != NULL);
				std::set<uint64_t>::iterator iter = DHTC->received_blocks.find(bad_block);
				if(iter != DHTC->received_blocks.end()){
					//found the server that sent the bad block, re_request all blocks from that server
					std::set<uint64_t>::iterator RB_iter_cur, RB_iter_end;
					RB_iter_cur = DHTC->received_blocks.begin();
					RB_iter_end = DHTC->received_blocks.end();
					while(RB_iter_cur != RB_iter_end){
						Request_Gen.force_re_request(*RB_iter_cur);
						++RB_iter_cur;
					}
					break;
				}
				++C_iter_cur;
			}

			//blacklist the server that sent the bad blocks
			DB_blacklist::add(DHTC->IP);

			if(DHTC == conn){
				//server that sent the bad block is the current server
				return false;
			}
		}else{
			//complete hash tree
			close_slots = true;
		}
	}

	if(download_complete){
		return false;
	}else if(!conn->slot_ID_requested){
		//slot_ID not yet obtained from server
		request = global::P_REQUEST_SLOT_HASH + hex::hex_to_binary(root_hash);
		conn->slot_ID_requested = true;
		expected.push_back(std::make_pair(global::P_SLOT_ID, global::P_SLOT_ID_SIZE));
		expected.push_back(std::make_pair(global::P_ERROR, 1));
		return true;
	}else if(!conn->slot_ID_received){
		//slot_ID requested but not yet received
		return false;
	}else if(close_slots && !conn->close_slot_sent){
		/*
		The file finished downloading or the download was manually stopped. This
		server has not been sent P_CLOSE_SLOT.
		*/
		request += global::P_CLOSE_SLOT;
		request += conn->slot_ID;
		conn->close_slot_sent = true;

		//the download is complete when all servers have been sent a P_CLOSE_SLOT
		bool unready_found = false;
		std::map<int, download_conn *>::iterator iter_cur, iter_end;
		iter_cur = Connection.begin();
		iter_end = Connection.end();
		while(iter_cur != iter_end){
			if(((download_hash_tree_conn *)iter_cur->second)->close_slot_sent == false){
				unready_found = true;
			}
			++iter_cur;
		}
		if(!unready_found){
			download_complete = true;
		}
		return true;
	}else if(Request_Gen.request(conn->latest_request)){
		//no request to be made at the moment
		request += global::P_SEND_BLOCK;
		request += conn->slot_ID;
		request += Convert_uint64.encode(conn->latest_request.back());
		int size;
		if(hash_tree_count - (conn->latest_request.back() * hashes_per_block) < hashes_per_block){
			//request will not yield full P_BLOCK
			size = (hash_tree_count - (conn->latest_request.back() * hashes_per_block)) * sha::HASH_LENGTH + 1;
		}else{
			size = global::P_BLOCK_SIZE;
		}
		expected.push_back(std::make_pair(global::P_BLOCK, size));
		expected.push_back(std::make_pair(global::P_ERROR, 1));
		return true;
	}else{
		return false;
	}
}

void download_hash_tree::response(const int & socket, std::string block)
{
	std::map<int, download_conn *>::iterator iter = Connection.find(socket);
	assert(iter != Connection.end());
	download_hash_tree_conn * conn = dynamic_cast<download_hash_tree_conn *>(iter->second);
	assert(conn != NULL);

	if(block[0] == global::P_SLOT_ID && conn->slot_ID_received == false){
		conn->slot_ID = block[1];
		conn->slot_ID_received = true;
	}else if(block[0] == global::P_BLOCK){
		//a block was received
		conn->Speed_Calculator.update(block.size()); //update server speed
		Speed_Calculator.update(block.size());       //update download speed
		block.erase(0, 1);                           //trim command
		Hash_Tree.write_hash(root_hash, conn->latest_request.front()*hashes_per_block, block);
		Request_Gen.fulfil(conn->latest_request.front());
		conn->received_blocks.insert(conn->latest_request.front());
		conn->latest_request.pop_front();
	}else{
		//server abusive
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

const bool & download_hash_tree::canceled()
{
	return _canceled;
}

const uint64_t & download_hash_tree::download_file_size()
{
	return _download_file_size;
}

const std::string & download_hash_tree::download_file_name()
{
	return _download_file_name;
}
