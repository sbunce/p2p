/*
This class handles rate limiting and can be used to get access to the average
upload and download rates. It is also used to set maximum upload/download rates.
This class is generally accessed through the reactor interface.
*/
//THREADSAFE
#ifndef H_NETWORK_RATE_LIMIT
#define H_NETWORK_RATE_LIMIT

//custom
#include "speed_calculator.hpp"

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//standard
#include <limits>

namespace network{
class rate_limit : private boost::noncopyable
{
public:
	rate_limit()
	{
		max_download(0);
		max_upload(0);
	}

	//add bytes to download average
	void add_download(const unsigned n_bytes)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		Download.add(n_bytes);
	}

	//add bytes to upload average
	void add_upload(const unsigned n_bytes)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		Upload.add(n_bytes);
	}

	/*
	Calculates how much we can send a number of sockets such that we stay under
	the max upload. Divides available upload between sockets. If no socket_count
	specified returns total available upload.
	Precondition: socket_count > 0
	*/
	int available_upload(const int socket_count = 1)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		return available_transfer(Upload, _max_upload, socket_count);
	}

	/*
	Calculates how much we can send a number of sockets such that we stay under
	the max download. Divides available upload between sockets. If no socket_count
	specified returns total available download.
	Precondition: socket_count > 0
	*/
	int available_download(const int socket_count = 1)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		return available_transfer(Download, _max_download, socket_count);
	}

	//returns average download rate
	unsigned download()
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		return Download.speed();
	}

	//returns max allowed download rate
	unsigned max_download()
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		return _max_download;
	}

	//sets max allowed download rate
	void max_download(const unsigned rate)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		if(rate == 0){
			_max_download = std::numeric_limits<unsigned>::max();
		}else{
			_max_download = rate;
		}
	}

	//returns max allowed upload rate
	unsigned max_upload()
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		return _max_upload;
	}

	//returns average upload rate
	unsigned upload()
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		return Upload.speed();
	}

	//sets max allowed upload rate
	void max_upload(const unsigned rate)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		if(rate == 0){
			_max_upload = std::numeric_limits<unsigned>::max();
		}else{
			_max_upload = rate;
		}
	}

private:
	//mutex for all public functions
	boost::recursive_mutex Recursive_Mutex;

	//max upload/download rate
	unsigned _max_download;
	unsigned _max_upload;

	speed_calculator Download;
	speed_calculator Upload;

	//calculates available_download or available_upload
	int available_transfer(speed_calculator & SC, const unsigned max_transfer,
		const int socket_count)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		assert(socket_count > 0);
		if(SC.current_second() >= max_transfer){
			return 0;
		}else{
			//calculate number of bytes to be divided among sockets
			unsigned transfer = max_transfer - SC.current_second();
			if(transfer < socket_count){
				//not all sockets will be allowed to transfer
				return 1;
			}else{
				//determine how many bytes one socket is allowed
				transfer = transfer / socket_count;

				//make sure we don't overflow int
				if(transfer > std::numeric_limits<int>::max()){
					return std::numeric_limits<int>::max();
				}else{
					return transfer;
				}
			}
		}
	}
};
}//end of network namespace
#endif
