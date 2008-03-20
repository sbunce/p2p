//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//std
#include <fstream>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <time.h>

#include "download_file.h"

download_file::download_file(std::string & file_hash_in, std::string & file_name_in, 
	std::string & file_path_in, unsigned long & file_size_in, unsigned int & latest_request_in,
	unsigned int & last_block_in, unsigned int & last_block_size_in,
	volatile bool * download_complete_flag_in)
{
	//non-defaults
	file_hash = file_hash_in;
	file_name = file_name_in;
	file_path = file_path_in;
	file_size = file_size_in;
	latest_request = latest_request_in;
	latest_written = latest_request_in - 1;
	last_block = last_block_in;
	last_block_size = last_block_size_in;
	download_complete_flag = download_complete_flag_in;

	//defaults
	download_complete = false;

	#ifdef DEBUG
	wasted_bytes = 0;
	#endif

	//set the hash type
	SHA.init(global::HASH_TYPE);
}

bool download_file::complete()
{
	return download_complete;
}

const std::string & download_file::hash()
{
	return file_hash;
}

const std::string & download_file::name()
{
	return file_name;
}

unsigned int download_file::percent_complete()
{
	return (int)(((latest_request * global::P_BLS_SIZE)/(float)file_size)*100);
}

bool download_file::request_choose_block(download_file_conn * conn)
{
	if(latest_written == last_block){
		download_complete = true;
		*download_complete_flag = true;
		return false;
	}

	//check for needed rerequests
	if(!rerequest.empty()){
		/*
		Only rerequest a block from a server that hasn't had a block it was supposed
		to serve rerequested. A rerequest is an indicator that a server is having
		problems so we do not trust it with a rerequest until it has caught up on
		the blocks it was supposed to send.
		*/
		std::deque<unsigned int>::iterator iter_cur, iter_end;
		iter_cur = conn->latest_request.begin();
		iter_end = conn->latest_request.end();
		while(iter_cur != iter_end){
			if(*iter_cur == *rerequest.begin()){
				return false;
			}
			++iter_cur;
		}

		global::debug_message(global::INFO,__FILE__,__FUNCTION__,"re-requesting block ",*rerequest.begin());

		conn->latest_request.push_back(*rerequest.begin());
		rerequest.erase(rerequest.begin());
		return true;
	}

	unsigned int optimal_next_block = 0;

	#ifdef SPEED_COMPENSATION
	unsigned int download_speed = Speed_Calculator.speed();
	unsigned int server_speed = conn->Speed_Calculator.speed();
	float block_lead = (float)download_speed/(float)server_speed;
	unsigned int optimal_lead = 0;

	//round .5 up
	if(block_lead - (int)block_lead > 0.5){
		optimal_lead = (unsigned int)(block_lead + 1);
	}
	else{
		optimal_lead = (unsigned int)block_lead;
	}

	if(!conn->latest_request.empty()){
		if(conn->latest_request.back() > latest_written){
			optimal_next_block = conn->latest_request.back() + optimal_lead;
		}
		else{
			optimal_next_block = latest_request + optimal_lead;
		}
	}
	else{
		optimal_next_block = latest_request + optimal_lead;
	}
	#endif

	if(optimal_next_block == 0){
		optimal_next_block = latest_written + 1;
	}

	//make sure optimal_last_block
	if(optimal_next_block > last_block){
		optimal_next_block = last_block;
	}

	/*
	If optimal next block has already been requested find the next closes block
	that hasn't been requested. Favor lower blocks over higher blocks if there is
	a tie.
	*/
	unsigned int request_lower = optimal_next_block;
	unsigned int request_upper = optimal_next_block;
	std::pair<std::set<unsigned int>::iterator, bool> Pair;
	while(true){

		//do not use numbers lower than beginning
		if(request_lower != (unsigned int)0-1 && request_lower != latest_written){
			Pair = requested_blocks.insert(request_lower);
			if(Pair.second){
				conn->latest_request.push_back(request_lower);
				latest_request = request_lower;
				return true;
			}
		}

		if(request_upper <= last_block){
			Pair = requested_blocks.insert(request_upper);
			if(Pair.second){
				conn->latest_request.push_back(request_upper);
				latest_request = request_upper;
				return true;
			}
		}
		else{
			return false;
		}

		//only go lower if not lower than beginning
		if(request_lower != (unsigned int)0-1 && request_lower != latest_written){
			--request_lower;
		}

		++request_upper;
	}
}

bool download_file::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected)
{
	std::map<int, download_conn *>::iterator iter = Connection.find(socket);
	download_file_conn * conn = (download_file_conn *)iter->second;

	if(iter == Connection.end()){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"socket not registered");
	}

	if(!request_choose_block(conn)){
		//no more requests to be made
		return false;
	}

	request = global::P_SBL + Conversion.encode_int(conn->file_ID) + Conversion.encode_int(conn->latest_request.back());

	int size;
	if(latest_request == last_block){
		size = last_block_size;
	}
	else{
		size = global::P_BLS_SIZE;
	}

	//expected response information
	expected.push_back(std::make_pair(global::P_BLS, size));
	expected.push_back(std::make_pair(global::P_DNE, global::P_DNE_SIZE));
	expected.push_back(std::make_pair(global::P_FNF, global::P_FNF_SIZE));

	return true;
}

void download_file::response(const int & socket, std::string block)
{
	//don't do anything if download is complete
	if(download_complete){
		return;
	}

	if(block[0] == global::P_BLS){
		//a block was received
		block.erase(0, 1); //trim command
		response_BLS(socket, block);
	}
	else if(block[0] == global::P_DNE){
		//server doesn't yet have this block

//DEBUG, the server has to be made unready for a certain time

		std::map<int, download_conn *>::iterator iter = Connection.find(socket);
		download_file_conn * conn = (download_file_conn *)iter->second;

		global::debug_message(global::INFO,__FILE__,__FUNCTION__,"received P_DNE from ",conn->server_IP);
	}
	else if(block[0] == global::P_FNF){
		//server is reporting that it doesn't have the file

//DEBUG, need to delete the IP/file ID associated from this file

		std::map<int, download_conn *>::iterator iter = Connection.find(socket);
		download_file_conn * conn = (download_file_conn *)iter->second;

		global::debug_message(global::INFO,__FILE__,__FUNCTION__,"received P_FNF from ",conn->server_IP);
	}
	else{
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"client_buffer passed a bad command");
	}
}

void download_file::response_BLS(const int & socket, std::string & block)
{
	//locate the server that this response is from
	std::map<int, download_conn *>::iterator iter = Connection.find(socket);
	download_file_conn * conn = (download_file_conn *)iter->second;

	if(iter == Connection.end()){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"socket not registered");
	}

	conn->Speed_Calculator.update(block.size()); //update server speed
	Speed_Calculator.update(block.size());       //update download speed

	received_blocks.insert(std::make_pair(conn->latest_request.front(), block));
	conn->latest_request.pop_front();

	//flush as much of the file block buffer as possible
	std::multimap<unsigned int, std::string>::iterator iter_cur, iter_end;
	iter_cur = received_blocks.begin();
	iter_end = received_blocks.end();
	while(iter_cur != iter_end){
		if(iter_cur->first == latest_written + 1){
			write_block(iter_cur->second);
			latest_written = iter_cur->first;
			requested_blocks.erase(iter_cur->first);

			//the block was received so there is no more need to rerequest it
			rerequest.erase(iter_cur->first);

			received_blocks.erase(iter_cur);
			iter_cur = received_blocks.begin();
		}
		else if(iter_cur->first <= latest_written){
			#ifdef DEBUG
			wasted_bytes += global::P_BLS_SIZE - 1;
			#endif
			received_blocks.erase(iter_cur);
			iter_cur = received_blocks.begin();
		}
		else{
			break;
		}
	}

	int check_interval = (Connection.size() * global::REREQUEST_FACTOR);
	if(received_blocks.size() % check_interval == 0 && received_blocks.size() != 0 && latest_written != (unsigned int)0 - 1){
		if(received_blocks.size() / check_interval == 1){
			//do not rerequest the block multiple times at this point
			if(rerequest.find(latest_written + 1) != rerequest.end()){
				rerequest.insert(latest_written + 1);
			}
		}
		else{
			//check for missing blocks in the oldest check_interval
			for(int x=latest_written + 1; x<latest_written + check_interval; ++x){
				if(rerequest.find(x) == rerequest.end()){
					rerequest.insert(x);
				}
			}
		}
	}

	//check if the download is complete
	if(latest_written == last_block){
		global::debug_message(global::INFO,__FILE__,__FUNCTION__,"wasted ",wasted_bytes/1024," kB");
		download_complete = true;
		*download_complete_flag = true;
	}
}

void download_file::stop()
{
	namespace fs = boost::filesystem;
	download_complete = true;
	fs::path path = fs::system_complete(fs::path(file_path, fs::native));
	fs::remove(path);
	*download_complete_flag = true;
}

void download_file::write_block(std::string & file_block)
{
	std::ofstream fout(file_path.c_str(), std::ios::app);
	if(fout.is_open()){
		fout.write(file_block.c_str(), file_block.size());
		fout.close();
	}
	else{
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"error opening file");
	}
}

const unsigned long & download_file::total_size()
{
	return file_size;
}
