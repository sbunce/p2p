#ifndef H_DOWNLOAD_FILE
#define H_DOWNLOAD_FILE

//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

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
		const uint64_t & file_size_in,
		const uint64_t & latest_request_in,
		const uint64_t & last_block_in,
		const unsigned int & last_block_size_in
	);

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual const std::string & hash();
	virtual const std::string & name();
	virtual unsigned int percent_complete();
	virtual bool request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected);
	virtual void response(const int & socket, std::string block);
	virtual void stop();
	virtual const uint64_t size();

private:
	/*
	After P_CLOSE_SLOT is sent to all servers and there are no pending responses
	from any server this should be set to true.
	*/
	bool download_complete;

	std::string file_hash;        //hash tree root hash
	std::string file_name;        //name of the file
	std::string file_path;        //path to write file to on local system
	uint64_t file_size;           //size of the file(bytes)
	uint64_t last_block;          //the last block number
	unsigned int last_block_size; //holds the exact size of the last fileBlock(in bytes)

	/*
	When the file has finished downloading this will be set to true to indicate
	that P_CLOSE_SLOT commands need to be sent to all servers.
	*/
	bool close_slots;

	/*
	check_complete - returns true if the download is ready to be removed
	writeTree      - writes a file block
	*/
	void write_block(uint64_t block_number, std::string & block);

	convert<uint64_t> Convert_uint64;
	request_gen Request_Gen;
	hash_tree Hash_Tree;
};
#endif
