#ifndef H_RATE_LIMIT
#define H_RATE_LIMIT

//boost
#include <boost/thread.hpp>

//custom
#include "speed_calculator.hpp"

//std
#include <limits>

class rate_limit
{
public:
	rate_limit(){}

	/*
	add_download_bytes    - add bytes to download speed average
	get_download_rate     - returns current download rate limit
	set_download_rate     - sets maximum download rate (minimum possible 1024 bytes)
	download_rate_control - given a maximum_possible_transfer size (in bytes),
	                        returns how many bytes can be sent to maintain rate
	add_upload_bytes      - add bytes to upload speed average
	get_upload_rate       - returns current upload rate limit
	set_upload_rate       - sets maximum download rate (minimum possible 1024 bytes)
	upload_rate_control   - given a maximum_possible_transfer size (in bytes),
	                        returns how many bytes can be recv'd to maintain rate
	*/
	static void add_download_bytes(const unsigned & n_bytes);
	static unsigned get_download_rate();
	static void set_download_rate(const unsigned & rate);
	static unsigned download_rate_control(const unsigned & max_possible_transfer);
	static void add_upload_bytes(const unsigned & n_bytes);
	static unsigned get_upload_rate();
	static void set_upload_rate(const unsigned & rate);
	static unsigned upload_rate_control(const unsigned & max_possible_transfer);

	/*
	download_speed - returns average download speed
	upload_speed   - returns average upload speed
	*/
	static unsigned download_speed();
	static unsigned upload_speed();

private:
	//mutex for all static public functions
	static boost::recursive_mutex Recursive_Mutex;

	static unsigned download_rate;
	static unsigned upload_rate;

	static speed_calculator Download;
	static speed_calculator Upload;
};
#endif
