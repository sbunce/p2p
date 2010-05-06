#include "rate_limit.hpp"

net::rate_limit::rate_limit()
{
	max_download(0);
	max_upload(0);
}

void net::rate_limit::add_download(const unsigned n_bytes)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	Download.add(n_bytes);
}

void net::rate_limit::add_upload(const unsigned n_bytes)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	Upload.add(n_bytes);
}

int net::rate_limit::available_upload(const int socket_count)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return available_transfer(Upload, _max_upload, socket_count);
}

int net::rate_limit::available_download(const int socket_count)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return available_transfer(Download, _max_download, socket_count);
}

unsigned net::rate_limit::download()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return Download.speed();
}

unsigned net::rate_limit::max_download()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return _max_download;
}

void net::rate_limit::max_download(const unsigned rate)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(rate == 0){
		_max_download = std::numeric_limits<unsigned>::max();
	}else{
		_max_download = rate;
	}
}

unsigned net::rate_limit::max_upload()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return _max_upload;
}

unsigned net::rate_limit::upload()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return Upload.speed();
}

void net::rate_limit::max_upload(const unsigned rate)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(rate == 0){
		_max_upload = std::numeric_limits<unsigned>::max();
	}else{
		_max_upload = rate;
	}
}

int net::rate_limit::available_transfer(speed_calc & SC,
	const unsigned max_transfer, const int socket_count)
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
