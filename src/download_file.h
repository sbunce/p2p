#ifndef H_DOWNLOADFILE
#define H_DOWNLOADFILE

#include <deque>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "atomic.h"
#include "conversion.h"
#include "download.h"
#include "super_block.h"
#include "global.h"

class download_file : public download
{
public:
	/*
	Download_complete_flag is a special bool used to signal that the client
	should check for the presence of a completed download.
	*/
	download_file(const std::string & file_hash_in, const std::string & file_name_in, 
		const std::string & file_path_in, const int & file_size_in, const int & latest_request_in,
		const int & last_block_in, const int & last_block_size_in, const int & last_super_block_in,
		const int & current_super_block_in, atomic<bool> * download_complete_flag_in);

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
	virtual const int & total_size();
	virtual void unreg_conn(const int & socket);

	//reg_conn - registers a server with this download
	void reg_conn(int socket, std::string & server_IP, unsigned int file_ID);

private:
	//used to hold information from a registered socket
	class connection
	{
	public:
		connection(std::string & server_IP_in, unsigned int file_ID_in)
		{
			server_IP = server_IP_in;
			file_ID = file_ID_in;
		}

		std::string server_IP;
		unsigned int file_ID;

		//the latest file block requested from this server
		unsigned int latest_request;
	};

	/*
	clientBuffers register/unregister with the download by storing information
	here. This is useful to make it easy to get at network information without
	having to store any network logic in the download.

	this maps the socket number to a connection
	*/
	std::map<int, connection> Connection;

	//creates message digests for superBlocks
	sha SHA;

	//these must be set before the download begins and will be set by ctor
	std::string file_hash;   //unique identifier of the file and message digest
	std::string file_name;   //name of the file
	std::string file_path;   //path to write file to on local system
	int file_size;           //size of the file(bytes)
	int latest_request;      //most recently requested block
	int last_block;          //the last block number
	int last_block_size;     //holds the exact size of the last fileBlock(in bytes)
	int last_super_block;    //holds the number of the last super_block
	int current_super_block; //what super_block clientBuffer is currently on

	/*
	Tells the client that a download is complete, used when download is complete.
	Full description in client.h.
	*/
	atomic<bool> * download_complete_flag;

	//true when the download is completed
	bool download_complete;

	//will hold 0, 1, or 2 super_blocks. This is manipulated by needed_block().
	std::deque<super_block> Super_Buffer;

	/*
	needed_block - returns number of a needed block
	writeTree    - writes a super_block to file
	*/
	unsigned int needed_block();
	void write_super_block(std::string container[]);
};
#endif

