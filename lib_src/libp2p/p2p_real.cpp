#include "p2p_real.hpp"

p2p_real::p2p_real()
{
	path::create_required_directories();

	//setup proxies for async getter/setter function calls
	max_connections_proxy = database::table::preferences::get_max_connections();
	max_download_rate_proxy = database::table::preferences::get_max_download_rate();
	max_upload_rate_proxy = database::table::preferences::get_max_upload_rate();

	Prime_Generator.start();
	Share.start();
}

p2p_real::~p2p_real()
{
	Prime_Generator.stop();
	Share.stop();
}

/*
void p2p_real::connect_call_back(network::sock & S)
{
}

void p2p_real::disconnect_call_back(network::sock & S)
{

}

void p2p_real::failed_connect_call_back(network::sock & S)
{

}
*/

void p2p_real::current_downloads(std::vector<download_status> & CD)
{

}

void p2p_real::current_uploads(std::vector<upload_status> & CU)
{

}

unsigned p2p_real::get_max_connections()
{
	return max_connections_proxy;
}

unsigned p2p_real::get_max_download_rate()
{
	return max_download_rate_proxy;
}

unsigned p2p_real::get_max_upload_rate()
{
	return max_upload_rate_proxy;
}

void p2p_real::pause_download(const std::string & hash)
{

}

unsigned p2p_real::prime_count()
{
	return Prime_Generator.prime_count();
}

boost::uint64_t p2p_real::share_size_bytes()
{
	return Share.size_bytes();
}

boost::uint64_t p2p_real::share_size_files()
{
	return Share.size_files();
}

//boost::bind can't handle default parameter, wrapper required
static void set_max_connections_wrapper(const unsigned max_connections)
{
	database::table::preferences::set_max_connections(max_connections);
}
void p2p_real::set_max_connections(const unsigned max_connections)
{
	max_connections_proxy = max_connections;
	thread_pool::singleton().queue(boost::bind(&set_max_connections_wrapper,
		max_connections));
}

//boost::bind can't handle default parameter, wrapper required
static void set_max_download_rate_wrapper(const unsigned max_download_rate)
{
	database::table::preferences::set_max_download_rate(max_download_rate);
}
void p2p_real::set_max_download_rate(const unsigned max_download_rate)
{
	max_download_rate_proxy = max_download_rate;
	thread_pool::singleton().queue(boost::bind(&set_max_download_rate_wrapper,
		max_download_rate));
}

//boost::bind can't handle default parameter, wrapper required
static void set_max_upload_rate_wrapper(const unsigned max_upload_rate)
{
	database::table::preferences::set_max_upload_rate(max_upload_rate);
}
void p2p_real::set_max_upload_rate(const unsigned max_upload_rate)
{
	max_upload_rate_proxy = max_upload_rate;
	thread_pool::singleton().queue(boost::bind(&set_max_upload_rate_wrapper,
		max_upload_rate));
}

void p2p_real::start_download(const download_info & DI)
{

}

void p2p_real::remove_download(const std::string & hash)
{

}

unsigned p2p_real::download_rate()
{
	return 123;
}

unsigned p2p_real::upload_rate()
{
	return 123;
}
