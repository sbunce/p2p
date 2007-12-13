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

	/*
	In addition to the pure virtuals the derived class must include a non-virtual
	reg_conn function. This function can be called in the client upon
	instantiation of the specific download to load information specific to the
	type of download.

	complete         - returns true if download complete (or if stop called)
	bytes_expected   - bytes expected in response to latest request
	hash             - returns a unique identifier to the download (uses SHA)
	IP_list          - appends all IPs registered with this download to list
	name             - returns the name of this download
	percent_complete - returns int 1 to 100
	request          - full request should be put in request
	                 - return false upon fatal error
	response         - response to the latest request passed to this
	                 - return false upon fatal error
	speed            - download speed in bytes per second
	stop             - causes download to stop (when stopped complete() = true)
	total_size       - the total amount needing to be downloaded
	unreg_conn       - unregisters a server with this download
	*/
	virtual bool complete() = 0;
	virtual const int bytes_expected() = 0;
	virtual const std::string & hash() = 0;
	virtual void IP_list(std::vector<std::string> & list) = 0;
	virtual const std::string & name() = 0;
	virtual int percent_complete() = 0;
	virtual bool request(const int & socket, std::string & request) = 0;
	virtual bool response(const int & socket, std::string & block) = 0;
	virtual const int & speed();
	virtual void stop() = 0;
	virtual const int & total_size() = 0;
	virtual void unreg_conn(const int & socket) = 0;

	//these vectors are parallel and used for download speed calculation
	int download_speed;               //speed of download(bytes per second)
	std::vector<int> Download_Second; //second at which Second_Bytes were downloaded
	std::vector<int> Second_Bytes;    //bytes in the second

protected:

	/*
	calculate_speed - calculates the download speed, should be called by response()
	*/
	void calculate_speed();
};
#endif
