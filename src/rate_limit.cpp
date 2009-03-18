#include "rate_limit.hpp"

rate_limit::rate_limit():
	max_download_rate(0),
	max_upload_rate(0),
	Upload(settings::SPEED_AVERAGE),
	Download(settings::SPEED_AVERAGE)
{

}

void rate_limit::add_download_bytes(const unsigned & n_bytes)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	Download.update(n_bytes);
}

void rate_limit::add_upload_bytes(const unsigned & n_bytes)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	Upload.update(n_bytes);
}

unsigned rate_limit::current_download_rate()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return Download.speed();
}

unsigned rate_limit::current_upload_rate()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return Upload.speed();
}

unsigned rate_limit::download_rate_control(const unsigned & max_possible_transfer)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	unsigned transfer = 0;
	if(Download.current_second() >= max_download_rate){
		//limit reached, send no bytes
		return transfer;
	}else{
		//limit not yet reached, determine how many bytes to send
		transfer = max_download_rate - Download.current_second();
		if(transfer > max_possible_transfer){
			transfer = max_possible_transfer;
		}
		return transfer;
	}
}

unsigned rate_limit::get_max_download_rate()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return max_download_rate;
}

unsigned rate_limit::get_max_upload_rate()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	return max_upload_rate;
}

void rate_limit::set_max_download_rate(const unsigned & rate)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(rate == 0){
		max_download_rate = 1;
	}else{
		max_download_rate = rate;
	}
}

void rate_limit::set_max_upload_rate(const unsigned & rate)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(rate == 0){
		max_upload_rate = 1;
	}else{
		max_upload_rate = rate;
	}
}

unsigned rate_limit::upload_rate_control(const unsigned & max_possible_transfer)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	unsigned transfer = 0;
	if(Upload.current_second() >= max_upload_rate){
		//limit reached, send no bytes
		return transfer;
	}else{
		//limit not yet reached, determine how many bytes to send
		transfer = max_upload_rate - Upload.current_second();
		if(transfer > max_possible_transfer){
			transfer = max_possible_transfer;
		}
		return transfer;
	}
}
