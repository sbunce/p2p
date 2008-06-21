/*
make a download_hash_tree_conn

it should contain a vector of blocks requested.

if a block fails from a server all blocks that were downloaded it should be
force_re_request'ed from other servers and the server disconnected.

*/

#include "download_hash_tree.h"

download_hash_tree::download_hash_tree(
	const std::string & root_hash_in,
	const uint64_t & file_size_in,
	const std::string & file_name_in
):
	root_hash(root_hash_in),
	file_size(file_size_in),
	file_name(file_name_in),
	download_complete(false),
	close_slots(false)
{
	assert((global::P_BLOCK_SIZE - 1) % sha::HASH_LENGTH == 0);
	hash_name = file_name + " HASH";
	hash_tree_count = hash_tree::block_hash_to_hash_tree_count(hash_tree::file_size_to_hash_count(file_size));
	hash_block_count = (hash_tree_count * sha::HASH_LENGTH) / (global::P_BLOCK_SIZE - 1);
	hashes_per_block = (global::P_BLOCK_SIZE - 1) / sha::HASH_LENGTH;
	if((hash_tree_count * sha::HASH_LENGTH) % (global::P_BLOCK_SIZE - 1) != 0){
		++hash_block_count;
	}

	std::pair<uint64_t, uint64_t> bad_hash;
	if(Hash_Tree.check_hash_tree(root_hash, hash_tree_count, bad_hash)){
std::cout << "RETURNED BAD HASH: " << bad_hash.first << "\n";
		//determine what hash_block the missing/bad hash falls within
		uint64_t start_hash_block = bad_hash.first / (global::P_BLOCK_SIZE - 1);
		Request_Gen.init(start_hash_block, hash_block_count - 1, global::RE_REQUEST_TIMEOUT);
	}else{

std::cout << "GOOD\n";
		//complete hash tree
		download_complete = true;
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

const std::string & download_hash_tree::name()
{
	return hash_name;
}

unsigned int download_hash_tree::percent_complete()
{
	return (unsigned int)(((float)Request_Gen.highest_requested() / (float)hash_block_count)*100);
}

bool download_hash_tree::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected)
{
	if(download_complete){
		return false;
	}

	download_hash_tree_conn * conn = (download_hash_tree_conn *)Connection[socket];

	if(!conn->slot_ID_requested){
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
	}

	if(Request_Gen.complete()){
		//all blocks requested, check tree
		std::pair<uint64_t, uint64_t> bad_hash;
std::cout << "htc: " << hash_tree_count << "\n";
		if(Hash_Tree.check_hash_tree(root_hash, hash_tree_count, bad_hash)){

std::cout << "bh: " << bad_hash.first << "\n";

			//figure out what block this is from and re-request
//			Request_Gen.force_re_request(bad_hash.first / hashes_per_block);
		}else{
			//complete hash tree
			close_slots = true;
		}
	}

	if(Request_Gen.request(conn->latest_request)){
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
	download_hash_tree_conn * conn = (download_hash_tree_conn *)Connection[socket];
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
		conn->latest_request.pop_front();
	}else{
		//server abusive
	}
}

void download_hash_tree::stop()
{
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
