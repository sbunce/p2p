#ifndef H_NET_RATE_LIMIT
#define H_NET_RATE_LIMIT

//custom
#include "speed_calc.hpp"

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>

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
	upload:
		Returns average upload rate (B/s).
	*/
	void add_download(const unsigned n_bytes);
	void add_upload(const unsigned n_bytes);
	int available_upload(const int socket_count = 1);
	int available_download(const int socket_count = 1);
	unsigned download();
	unsigned upload();

	/* Options
	get_max_download:
		Get maximum download rate (bytes/second).
	get_max_upload:
		Get maximum upload rate (bytes/second).
	set_max_download:
		Set maximum download rate (bytes/second).
	set_max_upload:
		Set maximum upload rate (bytes/second).
	*/
	unsigned get_max_download();
	unsigned get_max_upload();
	void set_max_download(const unsigned rate);
	void set_max_upload(const unsigned rate);

private:
	//mutex for all public functions
	boost::recursive_mutex Recursive_Mutex;

	//max upload/download rate
	unsigned max_download;
	unsigned max_upload;

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
