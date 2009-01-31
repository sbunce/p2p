#include "rate_limit.hpp"

atomic_int<unsigned> rate_limit::download_rate(global::DOWNLOAD_RATE);
atomic_int<unsigned> rate_limit::upload_rate(global::UPLOAD_RATE);
speed_calculator rate_limit::Download;
speed_calculator rate_limit::Upload;

rate_limit::rate_limit()
{

}

void rate_limit::add_download_bytes(const unsigned & n_bytes)
{
	Download.update(n_bytes);
}

void rate_limit::set_download_rate(const unsigned & rate)
{
	if(rate == 0){
		download_rate = 1;
	}else{
		download_rate = rate;
	}
}

unsigned rate_limit::get_download_rate()
{
	return download_rate;
}

unsigned rate_limit::download_rate_control(const unsigned & max_possible_transfer)
{
	unsigned transfer = 0;
	if(Download.current_second_bytes() >= download_rate){
		//limit reached, send no bytes
		return transfer;
	}else{
		//limit not yet reached, determine how many bytes to send
		transfer = download_rate - Download.current_second_bytes();
		if(transfer > max_possible_transfer){
			transfer = max_possible_transfer;
		}
		return transfer;
	}
}

void rate_limit::add_upload_bytes(const unsigned & n_bytes)
{
	Upload.update(n_bytes);
}

void rate_limit::set_upload_rate(const unsigned & rate)
{
	if(rate == 0){
		upload_rate = 1;
	}else{
		upload_rate = rate;
	}
}

unsigned rate_limit::get_upload_rate()
{
	return upload_rate;
}

unsigned rate_limit::upload_rate_control(const unsigned & max_possible_transfer)
{
	unsigned transfer = 0;
	if(Upload.current_second_bytes() >= upload_rate){
		//limit reached, send no bytes
		return transfer;
	}else{
		//limit not yet reached, determine how many bytes to send
		transfer = upload_rate - Upload.current_second_bytes();
		if(transfer > max_possible_transfer){
			transfer = max_possible_transfer;
		}
		return transfer;
	}
}

unsigned rate_limit::download_speed()
{
	return Download.speed();
}

unsigned rate_limit::upload_speed()
{
	return Upload.speed();
}
