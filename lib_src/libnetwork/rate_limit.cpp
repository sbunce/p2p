#include "rate_limit.hpp"

network::rate_limit::rate_limit():
	_max_download_rate(std::numeric_limits<unsigned>::max()),
	_max_upload_rate(std::numeric_limits<unsigned>::max())
{

}

void network::rate_limit::add_download_bytes(const unsigned n_bytes)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	Download.add(n_bytes);
}

void network::rate_limit::add_upload_bytes(const unsigned n_bytes)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	Upload.add(n_bytes);
}

int network::rate_limit::available_upload()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	unsigned transfer = 0;
	if(Upload.current_second() >= _max_upload_rate){
		//limit reached, send no bytes
		return transfer;
	}else{
		//limit not yet reached, determine how many bytes to send
		transfer = _max_upload_rate - Upload.current_second();
		if(transfer > std::numeric_limits<int>::max()){
			return std::numeric_limits<int>::max();
		}else{
			return transfer;
		}
	}
}

int network::rate_limit::available_download()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	unsigned transfer = 0;
	if(Download.current_second() >= _max_download_rate){
		//limit reached, send no bytes
		return transfer;
	}else{
		//limit not yet reached, determine how many bytes to send
		transfer = _max_download_rate - Download.current_second();
		if(transfer > std::numeric_limits<int>::max()){
			return std::numeric_limits<int>::max();
		}else{
			return transfer;
		}
	}
}

unsigned network::rate_limit::download_rate()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return Download.speed();
}

unsigned network::rate_limit::max_download_rate()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return _max_download_rate;
}

unsigned network::rate_limit::max_upload_rate()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return _max_upload_rate;
}

void network::rate_limit::max_download_rate(const unsigned rate)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(rate == 0){
		_max_download_rate = 1;
	}else{
		_max_download_rate = rate;
	}
}

void network::rate_limit::max_upload_rate(const unsigned rate)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(rate == 0){
		_max_upload_rate = 1;
	}else{
		_max_upload_rate = rate;
	}
}

unsigned network::rate_limit::upload_rate()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return Upload.speed();
}
