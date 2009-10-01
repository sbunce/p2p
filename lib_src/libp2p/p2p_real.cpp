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

	//test connection
	//Proactor.connect("topaz", settings::P2P_PORT);

	//get host_info for all hosts we need to connect to
	std::set<database::table::host::host_info> all_host;
	for(share::slot_iterator iter_cur = Share.begin_slot(), iter_end = Share.end_slot();
		iter_cur != iter_end; ++iter_cur)
	{
		iter_cur->merge_host(all_host);
	}

	//connect to all hosts
	for(std::set<database::table::host::host_info>::iterator iter_cur = all_host.begin(),
		iter_end = all_host.end(); iter_cur != iter_end; ++iter_cur)
	{
		Proactor.connect(iter_cur->host, iter_cur->port);
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

void p2p_real::start_download(const p2p::download & D)
{

}

unsigned p2p_real::supported_connections()
{
	return Proactor.supported_connections();
}

void p2p_real::remove_download(const std::string & hash)
{

}

void p2p_real::transfers(std::vector<p2p::transfer> & T)
{
	T.clear();
	for(share::slot_iterator iter_cur = Share.begin_slot(),
		iter_end = Share.end_slot(); iter_cur != iter_end; ++iter_cur)
	{
		p2p::transfer transfer;
		transfer.hash = iter_cur->hash();
		transfer.name = iter_cur->name();
		transfer.file_size = iter_cur->file_size();
		transfer.percent_complete = iter_cur->percent_complete();
		transfer.upload_speed = iter_cur->upload_speed();
		transfer.download_speed = iter_cur->download_speed();
		T.push_back(transfer);
	}
}

unsigned p2p_real::upload_rate()
{
	return Proactor.upload_rate();
}
