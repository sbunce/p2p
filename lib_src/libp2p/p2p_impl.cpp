#include "p2p_impl.hpp"

//BEGIN init
p2p_impl::init::init()
{
	db::init::create_all();
	LOG << "port: " << db::table::prefs::get_port() << " peer_ID: "
		<< convert::abbr(db::table::prefs::get_ID());
}
//END init

p2p_impl::p2p_impl():
	Thread_Pool(1),
	Share_Scanner(Connection_Manager)
{
	resume_thread = boost::thread(boost::bind(&p2p_impl::resume, this));
}

p2p_impl::~p2p_impl()
{
	Thread_Pool.stop_join();
	resume_thread.interrupt();
	resume_thread.join();
}

unsigned p2p_impl::connections()
{
	return Connection_Manager.connections();
}

unsigned p2p_impl::DHT_count()
{
	return Connection_Manager.DHT_count();
}

unsigned p2p_impl::get_max_connections()
{
	return db::table::prefs::get_max_connections();
}

unsigned p2p_impl::get_max_download_rate()
{
	return db::table::prefs::get_max_download_rate();
}

unsigned p2p_impl::get_max_upload_rate()
{
	return db::table::prefs::get_max_upload_rate();
}

void p2p_impl::resume()
{
	//delay helps GUI have responsive startup
	boost::this_thread::yield();

	//fill pref cache
	db::table::prefs::init_cache();

	//set prefs with proactor
	set_max_connections(db::table::prefs::get_max_connections());
	set_max_download_rate(db::table::prefs::get_max_download_rate());
	set_max_upload_rate(db::table::prefs::get_max_upload_rate());

	//repopulate share from database
	std::deque<db::table::share::info> resume = db::table::share::resume();
	while(!resume.empty()){
		file_info FI(
			resume.front().hash,
			resume.front().path,
			resume.front().file_size,
			resume.front().last_write_time
		);
		share::singleton().insert(FI);
		if(resume.front().file_state == db::table::share::downloading){
			//trigger slot creation for downloading file
			share::singleton().find_slot(FI.hash);
		}
		resume.pop_front();
	}

	//start scanning share
	Share_Scanner.start();

	//hash check resumed downloads
	for(share::slot_iterator it_cur = share::singleton().begin_slot(),
		it_end = share::singleton().end_slot(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->get_transfer()){
			it_cur->get_transfer()->check();
		}
	}

	//bring up networking
	Connection_Manager.start();

/* DEBUG, load all files in to DHT.
Eventually will need to be more selective about this. Or space it out.
*/
	for(share::const_file_iterator it_cur = share::singleton().begin_file(),
		it_end = share::singleton().end_file(); it_cur != it_end; ++it_cur)
	{
		Connection_Manager.store_file(it_cur->hash);
	}

	//find hosts which have files we need
	for(share::slot_iterator it_cur = share::singleton().begin_slot(),
		it_end = share::singleton().end_slot(); it_cur != it_end; ++it_cur)
	{
		Connection_Manager.add(it_cur->hash());
	}
}

static void set_max_connections_thread(const unsigned connections)
{
	db::table::prefs::set_max_connections(connections);
}

void p2p_impl::set_max_connections(const unsigned connections)
{
	//save extra 24 file descriptors for DB and misc other stuff
	assert(connections <= 1000);
	Connection_Manager.set_max_connections(connections / 2, connections / 2);
	Thread_Pool.enqueue(boost::bind(&set_max_connections_thread, connections));
}

static void set_max_download_rate_thread(const unsigned rate)
{
	db::table::prefs::set_max_download_rate(rate);
}

void p2p_impl::set_max_download_rate(const unsigned rate)
{
	Connection_Manager.set_max_download_rate(rate);
	Thread_Pool.enqueue(boost::bind(&set_max_download_rate_thread, rate));
}

static void set_max_upload_rate_thread(const unsigned rate)
{
	db::table::prefs::set_max_upload_rate(rate);
}

void p2p_impl::set_max_upload_rate(const unsigned rate)
{
	Connection_Manager.set_max_upload_rate(rate);
	Thread_Pool.enqueue(boost::bind(&set_max_upload_rate_thread, rate));
}

boost::uint64_t p2p_impl::share_size()
{
	return share::singleton().bytes();
}

boost::uint64_t p2p_impl::share_files()
{
	return share::singleton().files();
}

void p2p_impl::start_download(const p2p::download_info & DI)
{
	LOG;
}

void p2p_impl::remove_download(const std::string & hash)
{
	LOG << hash;
	Connection_Manager.remove(hash);
}

std::list<p2p::transfer_info> p2p_impl::transfer()
{
	std::list<p2p::transfer_info> tmp;
	for(share::slot_iterator it_cur = share::singleton().begin_slot(),
		it_end = share::singleton().end_slot(); it_cur != it_end; ++it_cur)
	{
		tmp.push_back(it_cur->info());
	}
	return tmp;
}

boost::optional<p2p::transfer_info> p2p_impl::transfer(const std::string & hash)
{
	share::slot_iterator it = share::singleton().find_slot(hash, false);
	if(it == share::singleton().end_slot()){
		return boost::optional<p2p::transfer_info>();
	}else{
		return it->info();
	}
}

unsigned p2p_impl::TCP_download_rate()
{
	return Connection_Manager.TCP_download_rate();
}

unsigned p2p_impl::TCP_upload_rate()
{
	return Connection_Manager.TCP_upload_rate();
}

unsigned p2p_impl::UDP_download_rate()
{
	return Connection_Manager.UDP_download_rate();
}

unsigned p2p_impl::UDP_upload_rate()
{
	return Connection_Manager.UDP_upload_rate();
}
