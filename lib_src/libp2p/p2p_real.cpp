#include "p2p_real.hpp"

p2p_real::p2p_real():
	Proactor(
		settings::P2P_PORT,
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

	//set preferences with the reactor
	Proactor.Reactor.max_connections(max_connections_proxy / 2, max_connections_proxy / 2);
	Proactor.Reactor.max_download_rate(max_download_rate_proxy);
	Proactor.Reactor.max_upload_rate(max_upload_rate_proxy);

//DEBUG, test
	Proactor.connect("127.0.0.1", settings::P2P_PORT);
}

p2p_real::~p2p_real()
{
	//stop threads
//change this to clear
	thread_pool::singleton().stop();
}

void p2p_real::connect_call_back(network::sock & S)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
		ret = Connection.insert(std::make_pair(S.socket_FD, new connection(S)));
	assert(ret.second);
	S.recv_call_back = boost::bind(&connection::recv_call_back, ret.first->second.get(), _1);
	S.send_call_back = boost::bind(&connection::send_call_back, ret.first->second.get(), _1);
}

void p2p_real::current_downloads(std::vector<download_status> & CD)
{
	CD.clear();

	download_status DS;
	DS.hash = "ABC";
	DS.name = "poik.flv";
	DS.size = 32768;
	DS.percent_complete = 50;
	DS.total_speed = 32768;
	DS.servers.push_back(std::make_pair("127.0.0.1", 16384));
	DS.servers.push_back(std::make_pair("127.0.0.2", 16384));
	CD.push_back(DS);

	DS.hash = "DEF";
	DS.name = "zort.flv";
	DS.size = 65535;
	DS.percent_complete = 75;
	DS.total_speed = 65535;
	CD.push_back(DS);
}

void p2p_real::current_uploads(std::vector<upload_status> & CU)
{
	CU.clear();

	upload_status US;
	US.hash = "ABC";
	US.IP = "127.0.0.1";
	US.size = 32768;
	US.path = "poik.avi";
	US.percent_complete = 50;
	US.speed = 32768;
	CU.push_back(US);

	US.hash = "DEF";
	US.IP = "127.0.0.1";
	US.size = 65535;
	US.path = "zort.avi";
	US.percent_complete = 75;
	US.speed = 65535;
	CU.push_back(US);
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
	return prime_generator::singleton().prime_count();
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
	return Proactor.Reactor.current_download_rate();
}

unsigned p2p_real::upload_rate()
{
	return Proactor.Reactor.current_upload_rate();
}
