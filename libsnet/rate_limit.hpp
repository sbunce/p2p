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
	rate_limit():
		max_download_rate(std::numeric_limits<unsigned>::max()),
		max_upload_rate(std::numeric_limits<unsigned>::max())
	{}

	//add bytes to download average
	void add_download_bytes(const unsigned n_bytes)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		Download.update(n_bytes);
	}

	//add bytes to upload average
	void add_upload_bytes(const unsigned n_bytes)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		Upload.update(n_bytes);
	}

	//returns current download rate (bytes/second)
	unsigned current_download_rate()
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		return Download.speed();
	}

	//returns current upload rate (bytes/second)
	unsigned current_upload_rate()
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		return Upload.speed();
	}

	/*
	Returns the number of bytes that can be received such that we don't go over
	max_download_rate.
	*/
	int download_rate_control()
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		unsigned transfer = 0;
		if(Download.current_second() >= max_download_rate){
			//limit reached, send no bytes
			return transfer;
		}else{
			//limit not yet reached, determine how many bytes to send
			transfer = max_download_rate - Download.current_second();
			if(transfer > std::numeric_limits<int>::max()){
				return std::numeric_limits<int>::max();
			}else{
				return transfer;
			}
		}
	}

	//returns maximun download rate allowed
	unsigned get_max_download_rate()
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		return max_download_rate;
	}

	//returns maximum upload rate allowed
	unsigned get_max_upload_rate()
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		return max_upload_rate;
	}

	//sets maximum download rate allowed
	void set_max_download_rate(const unsigned rate)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		if(rate == 0){
			max_download_rate = 1;
		}else{
			max_download_rate = rate;
		}
	}

	//sets maximum upload rate allowed
	void set_max_upload_rate(const unsigned rate)
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		if(rate == 0){
			max_upload_rate = 1;
		}else{
			max_upload_rate = rate;
		}
	}

	/*
	Returns the number of bytes that can be sent such that we don't go over
	max_upload_rate.
	*/
	int upload_rate_control()
	{
		boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
		unsigned transfer = 0;
		if(Upload.current_second() >= max_upload_rate){
			//limit reached, send no bytes
			return transfer;
		}else{
			//limit not yet reached, determine how many bytes to send
			transfer = max_upload_rate - Upload.current_second();
			if(transfer > std::numeric_limits<int>::max()){
				return std::numeric_limits<int>::max();
			}else{
				return transfer;
			}
		}
	}

private:
	//mutex for all static public functions
	boost::recursive_mutex Recursive_Mutex;

	unsigned max_download_rate;
	unsigned max_upload_rate;

	speed_calculator Download;
	speed_calculator Upload;
};
#endif
