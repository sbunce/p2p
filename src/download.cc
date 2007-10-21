//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//std
#include <fstream>
#include <iostream>
#include <sstream>
#include <time.h>

#include "download.h"

download::download(const std::string & hash_in, const std::string & file_name_in, 
	const std::string & file_path_in, const int & file_size_in, const int & latest_request_in,
	const int & last_block_in, const int & last_block_size_in, const int & last_super_block_in,
	const int & current_super_block_in, bool * download_complete_flag_in)
{
	//non-defaults
	hash = hash_in;
	file_name = file_name_in;
	file_path = file_path_in;
	file_size = file_size_in;
	latest_request = latest_request_in;
	last_block = last_block_in;
	last_block_size = last_block_size_in;
	last_super_block = last_super_block_in;
	current_super_block = current_super_block_in;
	download_complete_flag = download_complete_flag_in;

	//defaults
	download_complete = false;
	download_speed = 0;

#ifdef UNRELIABLE_CLIENT
	std::srand(time(0));
#endif

	//set the hash type
	SHA.Init(global::HASH_TYPE);
}

void download::add_block(const int & blockNumber, std::string & block)
{
	calculate_speed();

#ifdef UNRELIABLE_CLIENT
	int random = rand() % 100;
	if(random < global::UNRELIABLE_CLIENT_VALUE){
		std::cout << "testing: client::add_block(): OOPs! I dropped fileBlock " << blockNumber << "\n";
		if(blockNumber == last_block){
			block.clear();
		}
		else{
			block.erase(0, global::BUFFER_SIZE);
		}
	}
	else{
#endif

	//trim off protocol information
	block.erase(0, global::S_CTRL_SIZE);

	//add the block to the appropriate superBlock
	for(std::deque<superBlock>::iterator iter0 = Super_Buffer.begin(); iter0 != Super_Buffer.end(); ++iter0){
		if(iter0->add_block(blockNumber, block)){
			break;
		}
	}
#ifdef UNRELIABLE_CLIENT
	}
#endif

	//the Super_Buffer may be empty if the last block is requested more than once
	if(blockNumber == last_block && !Super_Buffer.empty()){
		write_super_block(Super_Buffer.back().container);
		Super_Buffer.pop_back();
		download_complete = true;
		*download_complete_flag = true;
	}
}

void download::calculate_speed()
{
	time_t currentTime = time(0);

	bool updated = false;
	//if the vectors are empty
	if(Download_Second.empty()){
		Download_Second.push_back(currentTime);
		Second_Bytes.push_back(global::BUFFER_SIZE);
		updated = true;
	}

	//get rid of information that's older than what to average
	if(currentTime - Download_Second.front() >= global::SPEED_AVERAGE + 2){
		Download_Second.erase(Download_Second.begin());
		Second_Bytes.erase(Second_Bytes.begin());
	}

	//if still on the same second
	if(!updated && Download_Second.back() == currentTime){
		Second_Bytes.back() += global::BUFFER_SIZE;
		updated = true;
	}

	//no entry for current second, add one
	if(!updated){
		Download_Second.push_back(currentTime);
		Second_Bytes.push_back(global::BUFFER_SIZE);
	}

	//count up all the bytes excluding the first and last second
	int totalBytes = 0;
	for(int x = 1; x <= Download_Second.size() - 1; ++x){
		totalBytes += Second_Bytes[x];
	}

	//divide by the time
	download_speed = totalBytes / global::SPEED_AVERAGE;
}

bool download::complete()
{
	return download_complete;
}

const int & download::get_bytes_expected()
{
	if(latest_request == last_block){
		return last_block_size;
	}
	else{
		return global::BUFFER_SIZE;
	}
}

const std::string & download::get_file_name()
{
	return file_name;
}

const int & download::get_file_size()
{
	return file_size;
}

const std::string & download::get_hash()
{
	return hash;
}

std::list<std::string> & download::get_IPs()
{
	get_IPs_list.clear();

	std::list<connection>::iterator iter_cur, iter_end;
	iter_cur = current_conns.begin();
	iter_end = current_conns.end();
	while(iter_cur != iter_end){
		get_IPs_list.push_back(iter_cur->server_IP);
		++iter_cur;
	}

	return get_IPs_list;
}

const int & download::get_latest_request()
{
	return latest_request;
}

const int download::get_request()
{
	/*
	Write out the oldest superBlocks if they're complete. It's possible the
	Super_Buffer could be empty so a check for this is done.
	*/
	if(!Super_Buffer.empty()){
		while(Super_Buffer.back().complete()){

			write_super_block(Super_Buffer.back().container);
			Super_Buffer.pop_back();

			if(Super_Buffer.empty()){
				break;
			}
		}
	}

	/*
	SB = superBlock
	FB = fileBlock
	* = requested fileBlock
	M = missing fileBlock

	Scenario #1 Ideal Conditions:
		P1 would load a SB and return the first request. P4 would serve all requests
	until SB was half completed. P3 would serve all requests until all requests
	made. At this point either P1 or P2 will get called depending on whether
	all FB were received for SB. This cycle would repeat under ideal conditions.

	Scenario #2 Unreliable Hosts:
		P1 would load a SB and return the first request. P4 would serve all requests
	until SB was half completed. P3 would serve all requests until all requests
	made. At this point we're missing FB that slow servers havn't sent yet.
	P2 adds a leading SB to the buffer. P4 requests FB for the leading SB until
	half the FB are requested. At this point P3 would rerequest FB from the back
	until those blocks were gotten. The back SB would be written as soon as it
	completed and P3 would start serving requests from the one remaining SB.

	Example of a rerequest:
	 back()                     front()
	-----------------------	   -----------------------
	|***M*******M*********| 	|***M******           |
	-----------------------   	-----------------------
	At this point the missing blocks from the back() would be rerequested.

	Example of new SB creation.
	 front() = back()
	-----------------------
	|*******************M*|
	-----------------------
	At this point a new leading SB would be added and that missing FB would be
	given time to arrive.
	*/
	if(Super_Buffer.empty()){ //P1
		Super_Buffer.push_front(superBlock(current_super_block++, last_block));
		latest_request = Super_Buffer.front().get_request();
	}
	else if(Super_Buffer.front().allRequested()){ //P2
		if(Super_Buffer.size() < 2 && current_super_block <= last_super_block){
			Super_Buffer.push_front(superBlock(current_super_block++, last_block));
		}
		latest_request = Super_Buffer.front().get_request();
	}
	else if(Super_Buffer.front().halfRequested()){ //P3
		latest_request = Super_Buffer.back().get_request();
	}
	else{ //P4
		latest_request = Super_Buffer.front().get_request();
	}

	/*
	Don't return the last block number until it's the absolute last needed by
	this download. This makes checking for a completed download a lot easier!
	*/
	if(latest_request == last_block){

		//determine if there are no superBlocks left but the last one
		if(Super_Buffer.size() == 1){

			/*
			If no superBlock but the last one remains then try getting another
			request. If the superBlock returns -1 then it doesn't need any other
			blocks.
			*/
			int request = Super_Buffer.back().get_request();

			//if no other block but the last one is needed
			if(request == -1){
				return last_block;
			}
			else{ //if another block is needed
				latest_request = request;
				return latest_request;
			}
		}
		else{ //more than one superBlock, serve request from oldest
			latest_request = Super_Buffer.back().get_request();
			return latest_request;
		}
	}
	else{ //any request but the last one gets returned without the above checking
		return latest_request;
	}
}

const int & download::get_speed()
{
	return download_speed;
}

void download::reg_conn(std::string & server_IP)
{
	connection temp(server_IP);
	current_conns.push_back(temp);
}

void download::unreg_conn(const std::string & server_IP)
{
	std::list<connection>::iterator iter_cur, iter_end;
	iter_cur = current_conns.begin();
	iter_end = current_conns.end();
	while(iter_cur != iter_end){
		if(server_IP == iter_cur->server_IP){
			current_conns.erase(iter_cur);
			break;
		}
		++iter_cur;
	}
}

void download::stop()
{
	namespace fs = boost::filesystem;

	download_complete = true;
	Super_Buffer.clear();
	fs::path path = fs::system_complete(fs::path(file_path, fs::native));
	fs::remove(path);
	*download_complete_flag = true;
}

void download::write_super_block(std::string container[])
{
#ifdef DEBUG_VERBOSE
	std::cout << "info: download::write_super_block() was called\n";
#endif
/*
	//get the messageDigest of the superBlock
	for(int x=0; x<global::SUPERBLOCK_SIZE; x++){
		SHA.Update((const sha_byte *) container[x].c_str(), container[x].size());
	}
	SHA.End();
	std::cout << SHA.StringHash() << "\n";
*/
	std::ofstream fout(file_path.c_str(), std::ios::app);

	if(fout.is_open()){

		for(int x=0; x<global::SUPERBLOCK_SIZE; ++x){
			fout.write(container[x].c_str(), container[x].size());
		}
		fout.close();
	}
	else{
#ifdef DEBUG
		std::cout << "error: download::writeTree() error opening file\n";
#endif
	}
}

