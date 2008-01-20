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

unsigned int download_file::bytes_expected()
{
	if(latest_request == last_block){
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

unsigned int download_file::percent_complete()
{
	return (int)(((latest_request * global::P_BLS_SIZE)/(float)file_size)*100);
}

bool download_file::request_choose_block(download_file_conn * conn)
{
//std::cout << "latest_written: " << latest_written << "\n";

	if(latest_written == last_block){
//std::cout << "request_choose_block signalling complete\n";
		download_complete = true;
		*download_complete_flag = true;
		return false;
	}

//DEBUG, server speed may need to be averaged over multiple packets, it's pretty bouncy, average it like download speed

	/*
	Determine what block to download by determining where the the download will be when the
	server will likely respond and picking a block close to that.
	*/
	unsigned int download_bytes_per_msec = (unsigned int)(download_speed / 1000);
	unsigned int server_bytes_per_msec = 0;
	if(conn->speed() != 0){
		server_bytes_per_msec = (unsigned int)(global::P_BLS_SIZE / conn->speed() / 1000);
	}

//std::cout << "download_bytes_per_msec: " << download_bytes_per_msec << "\n";
//std::cout << "server_bytes_per_msec:   " << server_bytes_per_msec << "\n";

	unsigned int next_block_lead = 1;
	if(server_bytes_per_msec != 0){
		next_block_lead = (int)(download_bytes_per_msec / server_bytes_per_msec) + 1;
	}
	unsigned int optimal_next_block = latest_written + next_block_lead;

	//make sure optimal_last_block
	if(optimal_next_block > last_block){
		optimal_next_block = last_block;
	}

//std::cout << "optimal_next_block: " << optimal_next_block << "\n";

/*
when I back off I rerequest stuff because when I'm backing off requested blocks may not
have been received
*/

	//attempt to find a block lower than or equal to optimal_next_block to request
	unsigned int request = optimal_next_block;
	std::pair<std::set<unsigned int>::iterator, bool> Pair;
	while(true){

//std::cout << "backtrack to: " << request << "\n";

		//(unsigned int)0 - 1 means we backed off past the first request number
		if(request == latest_written || request == (unsigned int)0 - 1){
//std::cout << "EXITING FIRST ON " << request << "\n";
			break;
		}

		Pair = requested_blocks.insert(request);

		if(Pair.second){
			conn->latest_request = request;
			latest_request = request;
			return true;
		}
		else{
			--request;
		}
	}

	//attempt to find a block higher than optimal_next_block to request
	request = optimal_next_block;
	while(true){

//std::cout << "forwardtrack to: " << request << "\n";

		if(request == last_block + 1){
//std::cout << "EXITING SECOND ON " << request << "\n";
			return false;
		}

		Pair = requested_blocks.insert(request);

		if(Pair.second){
			conn->latest_request = request;
			latest_request = request;
			return true;
		}
		else{
			++request;
		}
	}
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

	conn->time = timer::milli_time(); //marks the start time of the request
	request = global::P_SBL + conversion::encode_int(conn->file_ID) + conversion::encode_int(conn->latest_request);
	return true;
*/

	if(!request_choose_block(conn)){
		//download_complete, no request to be made
//std::cout << "NOMORE\n";
		return false;
	}

//std::cout << "latest_request: " << latest_request << "\n";
//std::cout << "last_block:     " << last_block << "\n";

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

	//std::cout << "server: " << conn->server_IP << " request: " << conn->latest_request << " time: " << timer::difference(conn->time, timer::milli_time()) << "ms\n";
	//conn->time = timer::difference(conn->time, timer::milli_time());
	conn->calculate_speed(global::P_BLS_SIZE);

	//trim off protocol information
	block.erase(0, 1);

	received_blocks.insert(std::make_pair(conn->latest_request, block));

	//flush as much of the file block buffer as possible
	std::map<unsigned int, std::string>::iterator iter_cur, iter_end;
	iter_cur = received_blocks.begin();
	iter_end = received_blocks.end();
	while(iter_cur != iter_end){
		if(iter_cur->first == latest_written + 1){
			write_block(iter_cur->second);
//std::cout << "writing: " << iter_cur->first << "\n";
			latest_written = iter_cur->first;
			requested_blocks.erase(iter_cur->first);
			received_blocks.erase(iter_cur);
			iter_cur = received_blocks.begin();
		}
		else{
//std::cout << "stuck on: " << iter_cur->first << "\n";
			break;
		}
	}

//by adding buffer sizes after processing of the buffer overall efficiency of requests can be determined
static unsigned int buffer_efficiency;
if(received_blocks.size() > 1){
	buffer_efficiency += received_blocks.size();
}


	//check if the download is complete
	if(latest_written == last_block){
std::cout << "buffer_efficiency: " << buffer_efficiency << "\n";
std::cout << "response signalling complete\n";
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
