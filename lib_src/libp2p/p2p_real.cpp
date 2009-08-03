#include "p2p_real.hpp"

p2p_real::p2p_real():
	Reactor(settings::P2P_PORT),
	Async_Resolve(Reactor),
	Proactor(
		Reactor,
		boost::bind(&p2p_real::connect_call_back, this, _1),
		boost::bind(&p2p_real::disconnect_call_back, this, _1),
		boost::bind(&p2p_real::failed_connect_call_back, this, _1)
	)
{
	//create directories and initialize database (if not already initialized)
	path::create_required_directories();
	database::init::all();

	//setup proxies for async getter/setter function calls
	max_connections_proxy = database::table::preferences::get_max_connections();
	max_download_rate_proxy = database::table::preferences::get_max_download_rate();
	max_upload_rate_proxy = database::table::preferences::get_max_upload_rate();

	//start prime generation threads
	Prime_Generator.start();

	//start share scan threads
	Share.start();

	//start networking threads
	Reactor.start();
	Async_Resolve.start();
	Proactor.start();
}

p2p_real::~p2p_real()
{
	//stop threads
	thread_pool::singleton().stop();
	Prime_Generator.stop();
	Share.stop();
	Reactor.stop();
	Async_Resolve.stop();
	Proactor.stop();
}

void p2p_real::connect_call_back(network::sock & S)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
		ret = Connection.insert(std::make_pair(S.socket_FD, new connection(S, Prime_Generator)));
	assert(ret.second);
	S.recv_call_back = boost::bind(&connection::recv_call_back, ret.first->second.get(), _1);
	S.send_call_back = boost::bind(&connection::send_call_back, ret.first->second.get(), _1);
}

void p2p_real::current_downloads(std::vector<download_status> & CD)
{

}

void p2p_real::current_uploads(std::vector<upload_status> & CU)
{

}

void p2p_real::disconnect_call_back(network::sock & S)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	if(Connection.erase(S.socket_FD) != 1){
		LOGGER << "disconnect called for socket that never connected";
		exit(1);
	}
}

void p2p_real::failed_connect_call_back(network::sock & S)
{
	LOGGER << "failed connect to " << S.socket_FD;
}

unsigned p2p_real::max_connections()
{
	return max_connections_proxy;
}

//boost::bind can't handle default parameter, wrapper required
static void max_connections_wrapper(const unsigned max_connections_in)
{
	database::table::preferences::set_max_connections(max_connections_in);
}
void p2p_real::max_connections(const unsigned connections)
{
	max_connections_proxy = connections;
	thread_pool::singleton().queue(boost::bind(&max_connections_wrapper,
		connections));
}

unsigned p2p_real::max_download_rate()
{
	return max_download_rate_proxy;
}

//boost::bind can't handle default parameter, wrapper required
static void max_download_rate_wrapper(const unsigned rate)
{
	database::table::preferences::set_max_download_rate(rate);
}
void p2p_real::max_download_rate(const unsigned rate)
{
	max_download_rate_proxy = rate;
	thread_pool::singleton().queue(boost::bind(&max_download_rate_wrapper,
		rate));
}

unsigned p2p_real::max_upload_rate()
{
	return max_upload_rate_proxy;
}

//boost::bind can't handle default parameter, wrapper required
static void max_upload_rate_wrapper(const unsigned rate)
{
	database::table::preferences::set_max_upload_rate(rate);
}
void p2p_real::max_upload_rate(const unsigned rate)
{
	max_upload_rate_proxy = rate;
	thread_pool::singleton().queue(boost::bind(&max_upload_rate_wrapper,
		rate));
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
