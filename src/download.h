#ifndef H_DOWNLOAD
#define H_DOWNLOAD

#include <deque>
#include <list>
#include <string>
#include <map>
#include <vector>

#include "atomic.h"
#include "download_conn.h"
#include "global.h"

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
	bytes_expected   - bytes expected in response to latest request
	hash             - returns a unique identifier to the download (uses SHA)
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
	virtual unsigned int bytes_expected() = 0;
	virtual const std::string & hash() = 0;

//I should not call this parameter a lit, it's confusing
	virtual void IP_list(std::vector<std::string> & list) = 0;

	virtual const std::string & name() = 0;
	virtual unsigned int percent_complete() = 0;
	virtual bool request(const int & socket, std::string & request) = 0;
	virtual bool response(const int & socket, std::string block) = 0;
	virtual const unsigned int & speed();
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

	/*
	calculate_speed - calculates the download speed, should be called by response()
	*/
	void calculate_speed(const unsigned int & packet_size);

	//speed of download(bytes per second)
	unsigned int download_speed;

private:
	//pair<second, bytes in second>
	std::deque<std::pair<unsigned int, unsigned int> > Download_Speed;
};
#endif
