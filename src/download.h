#ifndef H_DOWNLOAD
#define H_DOWNLOAD

#include <deque>
#include <list>
#include <string>
#include <vector>

#include "atomic.h"
#include "super_block.h"
#include "global.h"

class download
{
public:
	download(const std::string & hash_in, const std::string & file_name_in, 
		const std::string & file_path_in, const int & file_size_in, const int & latest_request_in,
		const int & last_block_in, const int & last_block_size_in, const int & last_super_block_in,
		const int & current_super_block_in, atomic<bool> * download_complete_flag_in);

	/*
	add_block          - add complete fileBlocks to the superBlocks
	completed          - returns true if download completed
	get_bytes_expected - returns the number of bytes to expect for the latest request
	get_file_name      - returns the file_name of the download
	get_file_size      - returns the file_size of the file being downloaded(bytes)
	get_hash           - returns the hash (UID) of this download
	get_IPs            - returns a list of all IPs this download associated with
	get_latest_request - returns latest_request
	getRequst          - returns the number of a needed block
	get_speed          - returns the speed of this download
	reg_conn           - registers a connection with the download
	unreg_conn         - unregisters a connection with the download
	stop               - stops the download by marking it as completed
	*/
	void add_block(const int & blockNumber, std::string & block);
	bool complete();
	const int & get_bytes_expected();
	const std::string & get_file_name();
	const int & get_file_size();
	const std::string & get_hash();
	std::list<std::string> & get_IPs();
	const int & get_latest_request();
	const int get_request();
	const int & get_speed();
	void reg_conn(std::string & server_IP);
	void unreg_conn(const std::string & server_IP);
	void stop();

private:
	//used by get_IPs
	std::list<std::string> get_IPs_list;

	//used to hold information from a registered socket
	class connection
	{
	public:
		connection(std::string & server_IP_in)
		{
			server_IP = server_IP_in;
		}

		std::string server_IP;
	};

	/*
	clientBuffers register/unregister with the download by storing information
	here. This is useful to make it easy to get at network information without
	having to store any network logic in the download.
	*/
	std::list<connection> current_conns;

	sha SHA; //creates message digests for superBlocks

	//these must be set before the download begins and will be set by ctor
	std::string hash;        //unique identifier of the file and message digest
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

	//used for speed calculation (automatically set in ctor)
	int download_speed;     //speed of download(bytes per second)
	bool download_complete; //true when the download is completed

	/*
	This will grow to SUPERBUFFER_SIZE if there are missing blocks in a super_block.
	This buffer is maintained to avoid the problem of rerequesting blocks that slow
	hosts havn't yet sent. Hopefully a slow host will finish sending it's block by
	the time SUPERBUFFER_SIZE superBlocks have completed. If it hasn't then it will
	get requested from a different host.
	*/
	std::deque<super_block> Super_Buffer;

	//these vectors are parallel and used for download speed calculation
	std::vector<int> Download_Second; //second at which Second_Bytes were downloaded
	std::vector<int> Second_Bytes;    //bytes in the second

	/*
	calculate_speed - calculates the download speed of a download
	writeTree       - writes a super_block to file
	*/
	void calculate_speed();
	void write_super_block(std::string container[]);
};
#endif

