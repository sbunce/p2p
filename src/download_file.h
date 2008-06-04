#ifndef H_DOWNLOAD_FILE
#define H_DOWNLOAD_FILE

//boost
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

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

//custom
#include "convert.h"
#include "download.h"
#include "download_file_conn.h"
#include "global.h"
#include "hex.h"
#include "request_gen.h"
#include "sha.h"

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
		const unsigned int & last_block_size_in,
		volatile int * download_file_complete_in
	);

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual const std::string & hash();
	virtual unsigned int max_response_size();
	virtual const std::string & name();
	virtual unsigned int percent_complete();
	virtual bool request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected);
	virtual void response(const int & socket, std::string block);
	virtual void stop();
	virtual const uint64_t & total_size();

private:
	std::string file_hash;        //unique identifier of the file and message digest
	std::string file_name;        //name of the file
	std::string file_path;        //path to write file to on local system
	uint64_t file_size;           //size of the file(bytes)
	uint64_t last_block;          //the last block number
	unsigned int last_block_size; //holds the exact size of the last fileBlock(in bytes)

	/*
	Tells the client that a download is complete, used when download is complete.
	Full description in client.h.
	*/
	volatile int * download_file_complete;

	//true when the download is completed
	bool download_complete;

	/*
	response_BLS         - processes a file block response
	writeTree            - writes a file block
	*/
	void response_BLS(const int & socket, std::string & block);
	void write_block(uint64_t block_number, std::string & block);

	convert<uint64_t> Convert_uint64;
	request_gen Request_Gen;
	sha SHA;
};
#endif
