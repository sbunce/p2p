/*
This class handles rate limiting and can be used to get access to the average
upload and download rates. It is also used to set maximum upload/download rates.
This class is generally accessed through the reactor interface.
*/
//THREADSAFE
#ifndef H_NETWORK_RATE_LIMIT
#define H_NETWORK_RATE_LIMIT

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <speed_calculator.hpp>

//standard
#include <limits>

namespace network{
class rate_limit : private boost::noncopyable
{
public:
	rate_limit();

	/*
	add_download_bytes:
		Add n_bytes to download rate.
	add_upload_bytes:
		Add n_bytes to upload rate.
	available_upload:
		Returns number of bytes that can be sent such that we don't go over
		_max_upload_rate.
	available_download:
		Returns number of bytes that can be received such that we don't go over
		_max_download_rate.
	download_rate:
		Returns average current download rate.
	max_download_rate:
		Returns maximum download rate.
	max_upload_rate:
		Returns maximum upload rate.
	max_download_rate:
		Sets maximum download rate.
	max_upload_rate:
		Sets maximum upload rate.
	upload_rate:
		Returns average current upload rate.
	*/
	void add_download_bytes(const unsigned n_bytes);
	void add_upload_bytes(const unsigned n_bytes);
	int available_upload();
	int available_download();
	unsigned download_rate();
	unsigned max_download_rate();
	unsigned max_upload_rate();
	void max_download_rate(const unsigned rate);
	void max_upload_rate(const unsigned rate);
	unsigned upload_rate();

private:
	//mutex for all public functions
	boost::recursive_mutex Recursive_Mutex;

	unsigned _max_download_rate;
	unsigned _max_upload_rate;

	speed_calculator Download;
	speed_calculator Upload;
};
}//end of network namespace
#endif
