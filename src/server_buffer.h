#ifndef H_SERVER_BUFFER
#define H_SERVER_BUFFER

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "encryption.h"
#include "global.h"
#include "speed_calculator.h"
#include "upload_info.h"

//std
#include <map>
#include <vector>

class server_buffer
{
public:
	server_buffer(const int & socket_FD_in, const std::string & IP_in);
	~server_buffer();

	std::string recv_buff; //buffer for partial recvs
	std::string send_buff; //buffer for partial sends

	int socket_FD;
	std::string IP;

	/*
	close_slot                   - close a slot opened with create_slot()
	create_slot_file             - create a download slot for a file
	                               returns true if slot successfully created, else false
	current_uploads              - adds upload information to the info vector
	path                         - sets path to uploading file path, returns false if slot invalid, else true
	update_slot_percent_complete - updates the percentage complete an upload is
	update_slot_speed            - updates speed for the upload the corresponds to the slot_ID
	*/
	void close_slot(char slot_ID);
	bool create_slot(char & slot_ID, const std::string & hash, const boost::uint64_t & size, const std::string & path);
	void current_uploads(std::vector<upload_info> & info);
	bool path(char slot_ID, std::string & path);
	void update_slot_percent_complete(char slot_ID, const boost::uint64_t & block_number);
	void update_slot_speed(char slot_ID, unsigned int bytes);

	bool exchange_key;               //true when key exchange happening
	std::string prime_remote_result; //holds prime and incoming result for key exchange
	encryption Encryption;

private:
	//mutex for all public functions
	boost::mutex Mutex;

	class slot_element
	{
	public:
		slot_element(
			const std::string & hash_in,
			const boost::uint64_t & size_in,
			const std::string & path_in
		):
			hash(hash_in),
			size(size_in),
			path(path_in),
			percent_complete(0)
		{
			assert(path.find_last_of("/") != std::string::npos);
			name = path.substr(path.find_last_of("/")+1);
		}

		std::string hash;
		boost::uint64_t size;
		std::string path;     //path of file associated with slot
		std::string name;     //name of file
		int percent_complete; //what % client has downloaded

		speed_calculator Speed_Calculator;
	};

	/*
	The array location is the slot_ID. The string holds the path to the file the
	slot is for. If the string is empty the slot is not yet taken.
	*/
	slot_element * Slot[256];
};
#endif
