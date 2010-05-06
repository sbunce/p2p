/*
This class handles rate limiting and can be used to get access to the average
upload and download rates. It is also used to set maximum upload/download rates.
This class is generally accessed through the reactor interface.
*/
#ifndef H_NET_RATE_LIMIT
#define H_NET_RATE_LIMIT

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <net/net.hpp>

//standard
#include <limits>

namespace net{
class rate_limit : private boost::noncopyable
{
public:
	rate_limit();

	/*
	add_download:
		Add bytes to download speed average.
	add_upload:
		Add bytes to upload speed average.
	available_upload:
		Calculates how much we can send a number of sockets such that we stay
		under the max upload. Divides available upload between sockets. If no
		socket_count specified returns total available upload.
		Precondition: socket_count > 0
	available_download:
		Calculates how much we can send a number of sockets such that we stay
		under the max download. Divides available upload between sockets. If no
		socket_count specified returns total available download.
		Precondition: socket_count > 0
	download:
		Returns average download rate (B/s).
	max_download:
		Get or set max allowed download rate (B/s).
	max_upload:
		Get or set max allowed upload rate (B/s).
	upload:
		Returns average upload rate (B/s).
	*/
	void add_download(const unsigned n_bytes);
	void add_upload(const unsigned n_bytes);
	int available_upload(const int socket_count = 1);
	int available_download(const int socket_count = 1);
	unsigned download();
	unsigned max_download();
	void max_download(const unsigned rate);
	unsigned max_upload();
	void max_upload(const unsigned rate);
	unsigned upload();

private:
	//mutex for all public functions
	boost::recursive_mutex Recursive_Mutex;

	//max upload/download rate
	unsigned _max_download;
	unsigned _max_upload;

	speed_calc Download;
	speed_calc Upload;

	/*
	available_transfer:
		Calculates available_download or available_upload.
	*/
	int available_transfer(speed_calc & SC, const unsigned max_transfer,
		const int socket_count);
};
}//end of namespace net
#endif
