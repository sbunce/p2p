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

download_file::download_file(const std::string & file_hash_in, const std::string & file_name_in, 
const std::string & file_path_in, const unsigned long & file_size_in, const unsigned int & latest_request_in,
const unsigned int & last_block_in, const unsigned int & last_block_size_in,
volatile int * download_file_complete_in)
: file_hash(file_hash_in), file_name(file_name_in), file_path(file_path_in),
file_size(file_size_in), last_block(last_block_in), last_block_size(last_block_size_in),
download_file_complete(download_file_complete_in), download_complete(false)
{
	SHA.init(global::HASH_TYPE);
	Request_Gen.init(latest_request_in, last_block, global::TIMEOUT);
}

bool download_file::complete()
{
	return download_complete;
}

const std::string & download_file::hash()
{
	return file_hash;
}

unsigned int download_file::max_response_size()
{
	return global::P_BLOCK_SIZE;
}

const std::string & download_file::name()
{
	return file_name;
}

unsigned int download_file::percent_complete()
{
	return (unsigned int)(((Request_Gen.highest_request() * global::P_BLOCK_SIZE)/(float)file_size)*100);
}

bool download_file::request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected)
{
	std::map<int, download_conn *>::iterator iter = Connection.find(socket);
	download_file_conn * conn = (download_file_conn *)iter->second;

	if(iter == Connection.end()){
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"socket not registered");
	}

	if(!Request_Gen.new_request(conn->latest_request)){
		//no request to be made at the moment
		return false;
	}

	request = global::P_SEND_BLOCK + Conversion.encode_int(conn->file_ID) + Conversion.encode_int(conn->latest_request.back());

	int size;
	if(conn->latest_request.back() == last_block){
		size = last_block_size;
	}else{
		size = global::P_BLOCK_SIZE;
	}

	//expected response information
	expected.push_back(std::make_pair(global::P_BLOCK, size));
	expected.push_back(std::make_pair(global::P_FILE_DOES_NOT_EXIST, global::P_FILE_DOES_NOT_EXIST_SIZE));
	expected.push_back(std::make_pair(global::P_FILE_NOT_FOUND, global::P_FILE_NOT_FOUND_SIZE));

	return true;
}

void download_file::response(const int & socket, std::string block)
{
	//don't do anything if download is complete
	if(download_complete){
		return;
	}

	if(block[0] == global::P_BLOCK){
		//a block was received
		block.erase(0, 1); //trim command
		response_BLS(socket, block);
	}else if(block[0] == global::P_FILE_DOES_NOT_EXIST){
		//server doesn't yet have this block
/*
DEBUG, the server has to be made unready for a certain time. This will have to
do with waiting until the server has the block.
*/

		std::map<int, download_conn *>::iterator iter = Connection.find(socket);
		download_file_conn * conn = (download_file_conn *)iter->second;

		global::debug_message(global::INFO,__FILE__,__FUNCTION__,"received P_DNE from ",conn->server_IP);
	}else if(block[0] == global::P_FILE_NOT_FOUND){
		//server is reporting that it doesn't have the file

//DEBUG, need to delete the IP/file ID associated from this file

		std::map<int, download_conn *>::iterator iter = Connection.find(socket);
		download_file_conn * conn = (download_file_conn *)iter->second;

		global::debug_message(global::INFO,__FILE__,__FUNCTION__,"received P_FNF from ",conn->server_IP);
	}else{
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

//hash check should happen here

	write_block(conn->latest_request.front(), block);
	Request_Gen.fulfilled(conn->latest_request.front());
	conn->latest_request.pop_front();

	//check if the download is complete
	if(Request_Gen.complete()){
		download_complete = true;
		++(*download_file_complete);
	}
}

void download_file::stop()
{
	namespace fs = boost::filesystem;
	download_complete = true;
	fs::path path = fs::system_complete(fs::path(file_path, fs::native));
	fs::remove(path);
	++(*download_file_complete);
}

void download_file::write_block(unsigned int block_number, std::string & block)
{
	std::fstream fout(file_path.c_str(), std::ios::in | std::ios::out | std::ios::binary);
	if(fout.is_open()){
		fout.seekp(block_number * (global::P_BLOCK_SIZE - 1), std::ios::beg);
		fout.write(block.c_str(), block.size());
	}else{
		global::debug_message(global::FATAL,__FILE__,__FUNCTION__,"error opening file");
	}
}

const unsigned long & download_file::total_size()
{
	return file_size;
}
