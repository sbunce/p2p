#ifndef H_DOWNLOAD
#define H_DOWNLOAD

//std
#include <deque>
#include <list>
#include <string>
#include <map>
#include <vector>

//custom
#include "atomic.h"
#include "download_conn.h"
#include "global.h"
#include "speed_calculator.h"

/*
Forward declaration for download_conn. A circular dependency exists between
download_conn and download.
*/
class download_conn;

class download
{
public:
	download();
	virtual ~download();

	/*
	complete         - returns true if download complete (or if stop called)
	hash             - returns a unique identifier to the download
	IP_list          - appends all IPs registered with this download to list
	name             - returns the name of this download
	percent_complete - returns int 1 to 100
	request          - full request should be put in request
	                 - return false upon fatal error
	response         - response to the latest request passed to this
	                 - return false if no request
	speed            - download speed in bytes per second
	stop             - causes download to stop (when stopped complete() = true)
	total_size       - the total amount needing to be downloaded
	*/
	virtual bool complete() = 0;
	virtual const std::string & hash() = 0;
	virtual void IP_list(std::vector<std::string> & list);
	virtual const std::string & name() = 0;
	virtual unsigned int percent_complete() = 0;
	virtual bool request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected) = 0;
	virtual void response(const int & socket, std::string block) = 0;
	virtual const unsigned int speed();
	virtual void stop() = 0;
	virtual const unsigned long & total_size() = 0;

	/*
	reg_conn   - registers a server with this download
	unreg_conn - unregisters a server with this download
	*/
	void reg_conn(const int & socket, download_conn * Download_conn);
	void unreg_conn(const int & socket);

protected:
	/*
	client_buffers register/unregister with the download by storing information
	here. If this is accessed within classes derived from download a typecast
	will need to be done to the specific derived type.

	This maps the socket number to a connection. Downloads can use this to store
	connection specific information. For example each server associated to a
	download_file may have a different file_ID.
	*/
	std::map<int, download_conn *> Connection;

	speed_calculator Speed_Calculator;
};
#endif
