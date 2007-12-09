/*
Download Interface Specification:

-The full response to the latest request is passed to this.
void response(string & response)

-Should return true when the download is complete(triggers download removal).
bool is_complete()

-How many bytes are expected in response to the latest request.
unsigned int bytes_expected()

-Size of the download.
unsigned int size()

-Name of the download.
string name()

-Unique ID of the download.
string hash()

-Return a string with a complete request(to be sent over wire).
string request()

-Current speed of the download in bytes/second.
unsigned int speed()

-When this is called the download needs to prepare to stop(is_complete should
return true when ready to stop). Do not signal complete until a full response 
to the last request was received, do not allow new requests.
void stop()

-When a new server is connected register with this function.
void reg_conn(std::string server_IP)

-When a server disconnects it will be unregisted with the download.
void unreg_conn(std::string server_IP)

-After a packet is received this function should be called with the parameter of 
how many bytes were received.
void calculate_speed(const int & bytes)


Missing Functionality
-The ability to handle errors, when a full response isn't sent. Perhaps make a
function to return the size of a potential error message.
*/

#ifndef H_DOWNLOAD
#define H_DOWNLOAD

#include <list>
#include <string>
#include <vector>

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
	virtual void response(const int & request_number, std::string & block) = 0;
	virtual bool complete() = 0;
	virtual const int & bytes_expected() = 0;
	virtual const std::string & get_file_name() = 0;
	virtual const int & get_file_size() = 0;
	virtual const std::string & get_hash() = 0;
	virtual const int & get_latest_request() = 0;
	virtual const int get_request() = 0;
	virtual const int & get_speed() = 0;
	virtual void stop() = 0;

	//these vectors are parallel and used for download speed calculation
	int download_speed;               //speed of download(bytes per second)
	std::vector<int> Download_Second; //second at which Second_Bytes were downloaded
	std::vector<int> Second_Bytes;    //bytes in the second

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
		connection(std::string & server_IP_in){ server_IP = server_IP_in; }
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

	/*non-pure virtuals that all downloads need
	calculate_speed - calculates the download speed
	*/
	virtual void calculate_speed();
};
#endif
