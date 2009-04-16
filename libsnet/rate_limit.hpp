//THREADSAFE
#ifndef H_RATE_LIMIT
#define H_RATE_LIMIT

//boost
#include <boost/thread.hpp>

//include
#include <speed_calculator.hpp>

//std
#include <limits>

class rate_limit
{
public:
	rate_limit();

	/*
	add_download_bytes:
		Add n_bytes to download rate.
	add_upload_bytes:
		Add n_bytes to upload rate.
	current_download_rate:
		Returns average current download rate.
	current_upload_rate:
		Returns average current upload rate.
	download_rate_control:
		Returns number of bytes that can be received such that we don't go over
		max_download_rate.
	get_max_download_rate:
		Returns maximum download rate.
	get_max_upload_rate:
		Returns maximum upload rate.
	set_max_download_rate:
		Sets maximum download rate.
	set_max_upload_rate:
		Sets maximum upload rate.
	upload_rate_control:
		Returns number of bytes that can be sent such that we don't go over
		max_upload_rate.
	*/
	void add_download_bytes(const unsigned n_bytes);
	void add_upload_bytes(const unsigned n_bytes);
	unsigned current_download_rate();
	unsigned current_upload_rate();
	int download_rate_control();
	unsigned get_max_download_rate();
	unsigned get_max_upload_rate();
	void set_max_download_rate(const unsigned rate);
	void set_max_upload_rate(const unsigned rate);
	int upload_rate_control();

private:
	//mutex for all static public functions
	boost::recursive_mutex Recursive_Mutex;

	unsigned max_download_rate;
	unsigned max_upload_rate;

	speed_calculator Download;
	speed_calculator Upload;
};
#endif
