#include "p2p_real.hpp"

p2p_real::p2p_real()
{
	path::create_required_directories();

	/* BEGIN STATIC INITIALIZATION
	The order these are specified in is important. The C++ standard specifies
	that order of destruction of static objects is the opposite of order of
	initialization. The singletons are static objects which get initialized when
	we first call the singleton() member function. We specify the order of
	initialization (and consequently destruction) based on their dependencies on
	other static objects.
	*/
	database::pool::singleton();
	prime_generator::singleton();
	//END STATIC INITIALIZATION
}

p2p_real::~p2p_real()
{

}

void p2p_real::current_downloads(std::vector<download_status> & CD)
{

}

void p2p_real::current_uploads(std::vector<upload_status> & CU)
{

}

unsigned p2p_real::get_max_connections()
{
	return 666;
}

unsigned p2p_real::get_max_download_rate()
{
	return 666;
}

unsigned p2p_real::get_max_upload_rate()
{
	return 666;
}

void p2p_real::pause_download(const std::string & hash)
{

}

unsigned p2p_real::prime_count()
{
	return prime_generator::singleton().prime_count();
}

void p2p_real::reconnect_unfinished()
{

}

boost::uint64_t p2p_real::share_size_bytes()
{
	return Share.size_bytes();
}

boost::uint64_t p2p_real::share_size_files()
{
	return Share.size_files();
}

void p2p_real::set_max_connections(const unsigned max_connections)
{

}

void p2p_real::set_max_download_rate(const unsigned max_download_rate)
{

}

void p2p_real::set_max_upload_rate(const unsigned max_upload_rate)
{

}

void p2p_real::start_download(const download_info & DI)
{

}

void p2p_real::remove_download(const std::string & hash)
{

}

unsigned p2p_real::download_rate()
{
	return 666;
}

unsigned p2p_real::upload_rate()
{
	return 666;
}
