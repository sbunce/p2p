#ifndef H_SERVER_BUFFER
#define H_SERVER_BUFFER

//boost
#include <boost/thread/mutex.hpp>

//custom
#include "global.h"

//std
#include <map>

class server_buffer
{
public:
	server_buffer(const int & socket_FD_in, const std::string & client_IP_in);
	~server_buffer();

	std::string recv_buff; //buffer for partial recvs
	std::string send_buff; //buffer for partial sends

	/*
	create_slot_file - create a download slot for a file
	                   returns true if slot successfully created, else false
	slot_valid       - returns true if a slot is valid, else false
	                   slot_valid() should be called before path()
	path             - returns the path associated with the slot
	*/
	bool create_slot(char & slot_ID, std::string file_path);
	bool slot_valid(char slot_ID);
	std::string & path(char slot_ID);

private:
	std::string client_IP;
	int socket_FD;

	/*
	The array location is the slot_ID. The string holds the path to the file the
	slot is for. If the string is empty the slot is not yet taken.
	*/
	std::string * Slot[256];
};
#endif
