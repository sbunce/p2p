#include "p2p_real.hpp"

//BEGIN init
p2p_real::init::init()
{
	db::init::create_all();
	LOG << "port: " << db::table::prefs::get_port() << " peer_ID: "
		<< db::table::prefs::get_ID();
}
//END init

p2p_real::p2p_real()
{
	resume_thread = boost::thread(boost::bind(&p2p_real::resume, this));
}

p2p_real::~p2p_real()
{
	resume_thread.interrupt();
	resume_thread.join();
}

unsigned p2p_real::download_rate()
{
	return Connection_Manager.Proactor.download_rate();
}

unsigned p2p_real::get_max_connections()
{
	return db::table::prefs::get_max_connections();
}

unsigned p2p_real::get_max_download_rate()
{
	return db::table::prefs::get_max_download_rate();
}

unsigned p2p_real::get_max_upload_rate()
{
	return db::table::prefs::get_max_upload_rate();
}

void p2p_real::resume()
{
	db::table::prefs::warm_up_cache();

	//delay helps GUI have responsive startup
	boost::this_thread::yield();

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

	//connect to peers that have files we need
	std::set<std::pair<std::string, std::string> >
		peers = db::table::join::resume_peers();
	for(std::set<std::pair<std::string, std::string> >::iterator
		it_cur = peers.begin(), it_end = peers.end(); it_cur != it_end;
		++it_cur)
	{
		Connection_Manager.Proactor.connect(it_cur->first, it_cur->second);
	}

	//bring up networking
	std::set<network::endpoint> E = network::get_endpoint(
		"",
		db::table::prefs::get_port(),
		network::tcp
	);
	assert(!E.empty());
	if(!Connection_Manager.Proactor.start(*E.begin())){
		LOG << "stub: handle failed listener start";
		exit(1);
	}
}

void p2p_real::set_max_connections(unsigned connections)
{
	//save extra 24 file descriptors for DB and misc other stuff
	assert(connections <= 1000);
	Connection_Manager.Proactor.set_connection_limit(connections / 2, connections / 2);
	db::table::prefs::set_max_connections(connections);
}

void p2p_real::set_max_download_rate(const unsigned rate)
{
	Connection_Manager.Proactor.set_max_download_rate(rate);
	db::table::prefs::set_max_download_rate(rate);
}

void p2p_real::set_max_upload_rate(const unsigned rate)
{
	Connection_Manager.Proactor.set_max_upload_rate(rate);
	db::table::prefs::set_max_upload_rate(rate);
}

boost::uint64_t p2p_real::share_size_bytes()
{
	return share::singleton().bytes();
}

boost::uint64_t p2p_real::share_size_files()
{
	return share::singleton().files();
}

void p2p_real::start_download(const p2p::download & D)
{
	LOG;
}

void p2p_real::remove_download(const std::string & hash)
{
	LOG << hash;
	Connection_Manager.remove(hash);
}

void p2p_real::transfers(std::vector<p2p::transfer> & T)
{
	T.clear();
	for(share::slot_iterator it_cur = share::singleton().begin_slot(),
		it_end = share::singleton().end_slot(); it_cur != it_end; ++it_cur)
	{
		it_cur->touch();
		p2p::transfer transfer;
		transfer.hash = it_cur->hash();
		transfer.name = it_cur->name();
		transfer.file_size = it_cur->file_size();
		transfer.percent_complete = it_cur->percent_complete();
		transfer.download_peers = it_cur->download_peers();
		transfer.download_speed = it_cur->download_speed();
		transfer.upload_peers = it_cur->upload_peers();
		transfer.upload_speed = it_cur->upload_speed();
		T.push_back(transfer);
	}
}

unsigned p2p_real::upload_rate()
{
	return Connection_Manager.Proactor.upload_rate();
}
