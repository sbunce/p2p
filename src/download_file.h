#ifndef H_DOWNLOAD_FILE
#define H_DOWNLOAD_FILE

#include <list>
#include <map>
#include <string>
#include <vector>

#include "atomic.h"
#include "conversion.h"
#include "download.h"
#include "download_file_conn.h"
#include "global.h"

class download_file : public download
{
public:
	/*
	Download_complete_flag is a special bool used to signal that the client
	should check for the presence of a completed download.
	*/
	download_file(std::string & file_hash_in, std::string & file_name_in, 
		std::string & file_path_in, unsigned long & file_size_in,
		unsigned int & latest_request_in, unsigned int & last_block_in,
		unsigned int & last_block_size_in, atomic<bool> * download_complete_flag_in);
	~download_file();

	//documentation for virtual functions in abstract base class
	virtual bool complete();
	virtual const int bytes_expected();
	virtual const std::string & hash();
	virtual void IP_list(std::vector<std::string> & list);
	virtual const std::string & name();
	virtual int percent_complete();
	virtual bool request(const int & socket, std::string & request);
	virtual bool response(const int & socket, std::string & block);
	virtual void stop();
	virtual const unsigned long & total_size();

private:
	//creates hashes for superBlocks
	sha SHA;

	//these must be set before the download begins and will be set by ctor
	std::string file_hash; //unique identifier of the file and message digest
	std::string file_name; //name of the file
	std::string file_path; //path to write file to on local system
	unsigned long file_size;      //size of the file(bytes)

	/*
	This variable is a bit tricky. It is incremented after storing it's value in
	a download_file_conn->latest_request so if you want to know the real
	latest_request after a request() has been run you need to do latest_request - 1.
	*/
	unsigned int latest_request;

	unsigned int next_request;    //next block to be requested
	long latest_written;          //most recently requested block
	unsigned int last_block;      //the last block number
	unsigned int last_block_size; //holds the exact size of the last fileBlock(in bytes)

	/*
	Tells the client that a download is complete, used when download is complete.
	Full description in client.h.
	*/
	atomic<bool> * download_complete_flag;

	//true when the download is completed
	bool download_complete;

	//maps received block numbers to the actual file block
	std::map<unsigned int, std::string> received_blocks;

	/*
	writeTree - writes a file block
	*/
	void write_block(std::string & file_block);
};
#endif

