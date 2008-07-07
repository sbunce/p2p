#ifndef H_DOWNLOAD
#define H_DOWNLOAD

//custom
#include "download_connection.h"
#include "download_info.h"
#include "global.h"
#include "speed_calculator.h"

//std
#include <deque>
#include <list>
#include <string>
#include <map>
#include <vector>

class download_connection; //circular dependency

class download
{
public:
	download();
	virtual ~download();

	/*
	complete              - returns true if download complete (or if stop called)
	hash                  - returns a unique identifier to the download
	info                  - returns Download_Info for this download (pushes on to vector)
	name                  - returns the name of this download
	percent_complete      - returns int 1 to 100
	register_connection   - registers a server with this download
	                        WARNING: If this is defined in derived, make it call base
	request               - full request should be put in request
	                      - return false upon fatal error
	response              - response to the latest request passed to this
	                      - return false if no request
	speed                 - download speed in bytes per second
	stop                  - cause download to stop
	size                  - the total amount needing to be downloaded
	unregister_connection - unregisters a server with this download
	                        WARNING: If this is defined in derived, make it call base
	visible               - override this and make it return true if the download should be visible in the GUI
	*/
	virtual bool complete() = 0;
	virtual const std::string & hash() = 0;
	virtual void IP_list(std::vector<std::string> & IP);
	virtual const std::string name() = 0;
	virtual unsigned int percent_complete() = 0;
	virtual void register_connection(const download_connection & DC);
	virtual bool request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected) = 0;
	virtual void response(const int & socket, std::string block) = 0;
	virtual unsigned int speed();
	virtual void stop() = 0;
	virtual const uint64_t size() = 0;
	virtual void unregister_connection(const int & socket);
	virtual bool visible();
//DEBUG, consider making a lot of these non-pure

protected:
	/*
	client_buffers register/unregister with the download by storing information
	here.

	This maps the socket number to the server IP.
	*/
	std::map<int, std::string> Connection;

	/*
	This can be used by the derived class to calculate speed for the individual
	download. Downloads may opt not to use this.
	*/
	speed_calculator Speed_Calculator;
};
#endif
