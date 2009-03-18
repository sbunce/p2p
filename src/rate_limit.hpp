#ifndef H_RATE_LIMIT
#define H_RATE_LIMIT

//boost
#include <boost/thread.hpp>

//custom
#include "settings.hpp"

//include
#include <singleton.hpp>
#include <speed_calculator.hpp>

//std
#include <limits>

class rate_limit: public singleton_base<rate_limit>
{
	friend class singleton_base<rate_limit>;
public:
	/*
	add_download_bytes     - add bytes to download speed average
	add_upload_bytes       - add bytes to upload speed average
	current_download_speed - returns average download rate
	current_upload_speed   - returns average upload rate
	download_rate_control  - given a maximum_possible_transfer size (in bytes),
	                         returns how many bytes can be sent to maintain rate
	get_max_download_rate  - returns current download rate limit
	get_max_upload_rate    - returns current upload rate limit
	set_max_download_rate  - sets maximum download rate (minimum possible 1024 bytes)
	set_max_upload_rate    - sets maximum download rate (minimum possible 1024 bytes)
	upload_rate_control    - given a maximum_possible_transfer size (in bytes),
	                         returns how many bytes can be recv'd to maintain rate
	*/
	void add_download_bytes(const unsigned & n_bytes);
	void add_upload_bytes(const unsigned & n_bytes);
	unsigned current_download_rate();
	unsigned current_upload_rate();
	unsigned download_rate_control(const unsigned & max_possible_transfer);
	unsigned get_max_download_rate();
	unsigned get_max_upload_rate();
	void set_max_download_rate(const unsigned & rate);
	void set_max_upload_rate(const unsigned & rate);
	unsigned upload_rate_control(const unsigned & max_possible_transfer);

private:
	rate_limit();

	//mutex for all static public functions
	boost::recursive_mutex Recursive_Mutex;

	unsigned max_download_rate;
	unsigned max_upload_rate;

	speed_calculator Download;
	speed_calculator Upload;
};
#endif
