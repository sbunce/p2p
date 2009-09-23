#include "p2p_real.hpp"

p2p_real::p2p_real():
	Share_Scan(Share),
	Connection_Manager(Share),
	Proactor(
		boost::bind(&connection_manager::connect_call_back, &Connection_Manager, _1),
		boost::bind(&connection_manager::disconnect_call_back, &Connection_Manager, _1),
		boost::bind(&connection_manager::failed_connect_call_back, &Connection_Manager, _1),
		false,
		settings::P2P_PORT
	),
	Thread_Pool(1)
{
	//setup proxies for async getter/setter function calls
	max_connections_proxy = database::table::preferences::get_max_connections();
	if(max_connections_proxy > Proactor.supported_connections()){
		max_connections_proxy = Proactor.supported_connections();
	}
	max_download_rate_proxy = database::table::preferences::get_max_download_rate();
	max_upload_rate_proxy = database::table::preferences::get_max_upload_rate();

	//set preferences with the proactor
	Proactor.max_connections(max_connections_proxy / 2, max_connections_proxy / 2);
	Proactor.max_download_rate(max_download_rate_proxy);
	Proactor.max_upload_rate(max_upload_rate_proxy);

	resume_thread = boost::thread(boost::bind(&p2p_real::resume, this));
}

p2p_real::~p2p_real()
{
	Thread_Pool.interrupt();
	Thread_Pool.join();
	resume_thread.interrupt();
	resume_thread.join();
}

unsigned p2p_real::download_rate()
{
	return Proactor.download_rate();
}

void p2p_real::downloads(std::vector<download_status> & Download)
{
	Download.clear();
	for(share::slot_iterator iter_cur = Share.begin_slot(), iter_end = Share.end_slot();
		iter_cur != iter_end; ++iter_cur)
	{
		if(!iter_cur->complete()){
			download_status DS;
			DS.hash = iter_cur->hash();
			DS.name = "foo";
			DS.size = iter_cur->file_size();
			DS.percent_complete = 69;
			DS.total_speed = 0;
			Download.push_back(DS);
		}
	}
}

unsigned p2p_real::max_connections()
{
	return max_connections_proxy;
}

void p2p_real::max_connections(unsigned connections)
{
	if(connections > Proactor.supported_connections()){
		connections = Proactor.supported_connections();
	}
	Proactor.max_connections(connections / 2, connections / 2);
	max_connections_proxy = connections;
	Thread_Pool.queue(boost::bind(&database::table::preferences::set_max_connections,
		connections, database::pool::get_proxy()));
}

unsigned p2p_real::max_download_rate()
{
	return max_download_rate_proxy;
}

void p2p_real::max_download_rate(const unsigned rate)
{
	Proactor.max_download_rate(rate);
	max_download_rate_proxy = rate;
	Thread_Pool.queue(boost::bind(&database::table::preferences::set_max_download_rate,
		rate, database::pool::get_proxy()));
}

unsigned p2p_real::max_upload_rate()
{
	return max_upload_rate_proxy;
}

void p2p_real::max_upload_rate(const unsigned rate)
{
	Proactor.max_upload_rate(rate);
	max_upload_rate_proxy = rate;
	Thread_Pool.queue(boost::bind(&database::table::preferences::set_max_upload_rate,
		rate, database::pool::get_proxy()));
}

void p2p_real::pause_download(const std::string & hash)
{

}

unsigned p2p_real::prime_count()
{
	return prime_generator::singleton().prime_count();
}

void p2p_real::resume()
{
	Share_Scan.block_until_resumed();
	Proactor.start();
	LOGGER << "share scan resumed";
	for(share::slot_iterator iter_cur = Share.begin_slot(), iter_end = Share.end_slot();
		iter_cur != iter_end; ++iter_cur)
	{

	}
}

boost::uint64_t p2p_real::share_size_bytes()
{
	return Share.bytes();
}

boost::uint64_t p2p_real::share_size_files()
{
	return Share.files();
}

void p2p_real::start_download(const download_info & DI)
{

}

unsigned p2p_real::supported_connections()
{
	return Proactor.supported_connections();
}

void p2p_real::remove_download(const std::string & hash)
{

}

unsigned p2p_real::upload_rate()
{
	return Proactor.upload_rate();
}

void p2p_real::uploads(std::vector<upload_status> & Upload)
{
	Upload.clear();
	for(share::slot_iterator iter_cur = Share.begin_slot(), iter_end = Share.end_slot();
		iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->complete()){
			upload_status US;
			US.hash = iter_cur->hash();
			US.path = "/foo/bar";
			US.size = iter_cur->file_size();
			US.percent_complete = 69;
			US.speed = 0;
			Upload.push_back(US);
		}
	}
}
