#ifndef H_DOWNLOAD
#define H_DOWNLOAD

//custom
#include "download_connection.h"
#include "global.h"
#include "speed_calculator.h"

//std
#include <list>
#include <string>
#include <map>
#include <vector>

class download
{
public:
	download();
	virtual ~download();

	enum mode { BINARY_MODE, TEXT_MODE, NO_REQUEST };

	/*
	If the visible() function returns true the client sends information from all
	the functions in this section to the GUI.

	hash                  - returns a unique identifier to the download (may be a hash)
	name                  - returns the name of this download
	IP_list               - returns a vector of IP addresses the download is downloading from
	percent_complete      - should return integer 0 to 100 to indicate percent complete
	speed                 - download speed in bytes per second
	size                  - the total amount needing to be downloaded
	*/
	virtual bool visible();
	virtual const std::string hash();
	virtual void IP_list(std::vector<std::string> & IP);
	virtual const std::string name();
	virtual unsigned int percent_complete();
	virtual unsigned int speed();
	virtual const uint64_t size();

	/*
	If server specific information needs to be stored in the download these can
	be overridden. If you do override these you must call the register and unregister
	functions in the base class. (see download_file for example)
	*/
	virtual void register_connection(const download_connection & DC);
	virtual void unregister_connection(const int & socket);

	/*
	Minimum functions which need to be defined for a download.

	complete - if true is returned the download is removed
	request  - make a request of a server, expected vector is possible response commands paired with length of responses
	response - responses to requests are sent to this function once the entire response is gotten
	stop     - this must prepare the download for early termination
	*/
	virtual bool complete() = 0;
	virtual mode request(const int & socket, std::string & request, std::vector<std::pair<char, int> > & expected) = 0;
	virtual void response(const int & socket, std::string block) = 0;
	virtual void stop() = 0;

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
