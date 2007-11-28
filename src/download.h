#ifndef H_DOWNLOAD
#define H_DOWNLOAD

#include <list>
#include <string>

#include "atomic.h"
#include "global.h"

class download
{
public:
	download();

	/* pure virtuals which must be overloaded by derived classes
	add_block          - add complete fileBlocks to the superBlocks
	completed          - returns true if download completed
	get_bytes_expected - returns the number of bytes to expect for the latest request
	get_file_name      - returns the file_name of the download
	get_file_size      - returns the file_size of the file being downloaded(bytes)
	get_hash           - returns the hash (UID) of this download
	get_latest_request - returns latest_request
	getRequst          - returns the number of a needed block
	get_speed          - returns the speed of this download

	stop               - stops the download by marking it as completed
	*/
	virtual void add_block(const int & blockNumber, std::string & block) = 0;
	virtual bool complete() = 0;
	virtual const int & get_bytes_expected() = 0;
	virtual const std::string & get_file_name() = 0;
	virtual const int & get_file_size() = 0;
	virtual const std::string & get_hash() = 0;
	virtual const int & get_latest_request() = 0;
	virtual const int get_request() = 0;
	virtual const int & get_speed() = 0;
	virtual void stop() = 0;

	/* non-pure virtuals which all downloads need
	get_IPs    - returns a list of IPs associated with this download
	reg_conn   - registers a connection with the download
	unreg_conn - unregisters a connection with the download
	*/
	virtual std::list<std::string> & get_IPs();
	virtual void reg_conn(std::string & server_IP);
	virtual void unreg_conn(const std::string & server_IP);

protected:

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

	//used by get_IPs
	std::list<std::string> get_IPs_list;
};
#endif
