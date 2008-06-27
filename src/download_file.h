#ifndef H_DOWNLOAD_FILE
#define H_DOWNLOAD_FILE

//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

//custom
#include "convert.h"
#include "download.h"
#include "download_file_conn.h"
#include "global.h"
#include "hash_tree.h"
#include "hex.h"
#include "request_gen.h"

//std
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

class download_file : public download
{
public:
	/*
	download_file_complete is a special bool used to signal that the client
	should check for the presence of a completed download.
	*/
	download_file(
		const std::string & file_hash_in,
		const std::string & file_name_in, 
		const std::string & file_path_in,
		const uint64_t & file_size_in
	);

	~download_file();

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual const std::string & hash();
	virtual const std::string name();
	virtual unsigned int percent_complete();
	virtual bool request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected);
	virtual void response(const int & socket, std::string block);
	virtual void stop();
	virtual const uint64_t size();

private:
	/*
	When the download_file is constructed a thread is spawned to hash check a
	partial download that has been resumed. Requests are only returned when hashing
	is done which is indicated by this being set equal to false.
	*/
	volatile bool hashing;

	/*
	These variables are for tracking the progress of the hash checking so a percent
	can be calculated.
	*/
	volatile uint64_t hash_latest;
	uint64_t hash_last;

	volatile int threads;       //one if hash checking, otherwise zero
	volatile bool stop_threads; //if set to true hash checking will stop early

	/*
	These variables are only to be used by the hash_check() thread.
	*/
	std::string thread_file_hash;
	std::string thread_file_path;

	/*
	After P_CLOSE_SLOT is sent to all servers and there are no pending responses
	from any server this should be set to true.
	*/
	bool download_complete;

	std::string file_hash;        //hash tree root hash
	std::string file_name;        //name of the file
	std::string file_path;        //path to write file to
	uint64_t file_size;           //size of the file(bytes)
	uint64_t last_block;          //the last block number
	unsigned int last_block_size; //holds the exact size of the last fileBlock(in bytes)

	/*
	When the file has finished downloading this will be set to true to indicate
	that P_CLOSE_SLOT commands need to be sent to all servers.
	*/
	bool close_slots;

	/*
	hash_check  - checks partial download integrity (run in thread spawned in ctor)
	write_block - writes a file block
	*/
	void hash_check();
	void write_block(uint64_t block_number, std::string & block);

	convert<uint64_t> Convert_uint64;
	request_gen Request_Gen;
	hash_tree Hash_Tree;
};
#endif
