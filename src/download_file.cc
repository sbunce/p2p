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
	atomic<bool> * download_complete_flag_in)
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

#ifdef UNRELIABLE_CLIENT
	std::srand(time(0));
#endif

	//set the hash type
	SHA.Init(global::HASH_TYPE);
}

download_file::~download_file()
{

}

bool download_file::complete()
{
	return download_complete;
}

const int download_file::bytes_expected()
{
	if(latest_request - 1 == last_block){
		return last_block_size;
	}
	else{
		return global::P_BLS_SIZE;
	}
}

const std::string & download_file::hash()
{
	return file_hash;
}

void download_file::IP_list(std::vector<std::string> & list)
{
	std::map<int, download_conn *>::iterator iter_cur, iter_end;
	iter_cur = Connection.begin();
	iter_end = Connection.end();
	while(iter_cur != iter_end){
		list.push_back(iter_cur->second->server_IP);
		++iter_cur;
	}
}

const std::string & download_file::name()
{
	return file_name;
}

int download_file::percent_complete()
{
	return (int)(((latest_request * global::P_BLS_SIZE)/(float)file_size)*100);
}

bool download_file::request(const int & socket, std::string & request)
{
	std::map<int, download_conn *>::iterator iter = Connection.find(socket);
	download_file_conn * conn = (download_file_conn *)iter->second;

	if(iter == Connection.end()){
		std::cout << "fatal error: download_file::request() socket not registered";
		exit(1);
	}

	/*
	If the received_blocks buffer is large that means it can't be flushed to disk
	because a file block is missing. In this case a rerequest is done.
	*/
/*
	int increment = Connection.size() * download_speed
	if(received_blocks.size() % increment){

	}
*/

	//check if last block already requested
	if(latest_request - 1 == last_block){
		if(latest_written == last_block){
			//download complete, don't make request and signal complete
			download_complete = true;
			*download_complete_flag = true;
			return false;
		}
		conn->latest_request = last_block;
	}
	else{
		//last block not yet reached
		conn->latest_request = latest_request++;
	}

	request = global::P_SBL + conversion::encode_int(conn->file_ID) + conversion::encode_int(conn->latest_request);
	return true;
}

bool download_file::response(const int & socket, std::string & block)
{
	#ifdef UNRELIABLE_CLIENT
	if(std::rand() % 100 == 0){
		return false;
	}
	#endif

	//locate the server that this response is from
	std::map<int, download_conn *>::iterator iter = Connection.find(socket);
	download_file_conn * conn = (download_file_conn *)iter->second;

	if(iter == Connection.end()){
		std::cout << "fatal error: download_file::response() socket not registered\n";
		exit(1);
	}

	//trim off protocol information
	block.erase(0, 1);

	if(conn->latest_request == latest_written + 1){
		//blocks is next to be written, it doesn't need to be buffered
		write_block(block);
		latest_written = conn->latest_request;
	}
	else if(conn->latest_request <= latest_written){
		//this server was too late in responding, request fulfilled by another server already

//DEBUG, possibly keep track of "late" servers

		return false;
	}
	else{
		//add block to buffer
		std::pair<std::map<unsigned int, std::string>::iterator, bool> latest_block = received_blocks.insert(std::make_pair(conn->latest_request, block));

		//if insertion failed no need to continue
		if(latest_block.second == false){
			return false;
		}
	}

	//flush as much of the file block buffer as possible
	std::map<unsigned int, std::string>::iterator iter_cur, iter_end;
	iter_cur = received_blocks.begin();
	iter_end = received_blocks.end();
	while(iter_cur != iter_end){
		if(iter_cur->first == latest_written + 1){
			write_block(iter_cur->second);
			latest_written = iter_cur->first;
			received_blocks.erase(iter_cur);
			iter_cur = received_blocks.begin();
		}
		else{
			break;
		}
	}

	//check if the download is complete
	if(latest_written == last_block){
		download_complete = true;
		*download_complete_flag = true;
	}

	calculate_speed(global::P_BLS_SIZE);
	return true;
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
#ifdef DEBUG
		std::cout << "error: download_file::writeTree() error opening file\n";
#endif
	}
}

const unsigned long & download_file::total_size()
{
	return file_size;
}
