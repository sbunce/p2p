#include "connection_manager.hpp"

//BEGIN connection_element
connection_manager::connection_element::connection_element(
	const net::endpoint & ep_in,
	const boost::shared_ptr<connection> & Connection_in
):
	ep(ep_in),
	Connection(Connection_in)
{

}

connection_manager::connection_element::connection_element(const connection_element & CE):
	ep(CE.ep),
	Connection(CE.Connection)
{

}
//END connection_element

connection_manager::connection_manager():
	Proactor(
		boost::bind(&connection_manager::connect_call_back, this, _1),
		boost::bind(&connection_manager::disconnect_call_back, this, _1)
	),
	Thread_Pool(1)
{

}

connection_manager::~connection_manager()
{
	Thread_Pool.stop();
	Thread_Pool.clear();
	DHT.stop();
	Proactor.stop();
}

void connection_manager::add(const std::string & hash)
{
	LOG << hash;
	DHT.find_file(hash, boost::bind(&connection_manager::connect, this, _1, hash));
}

void connection_manager::connect(const net::endpoint ep, const std::string hash)
{
	boost::mutex::scoped_lock lock(Connect_mutex);
	if(ep.IP() == "127.0.0.1" && ep.port() == db::table::prefs::get_port()){
		LOG << "received peer message to connect to self";
		return;
	}
	bool connecting = Connecting.find(ep) != Connecting.end();
	bool connected = Connected.find(ep) != Connected.end();
	if(!connecting && !connected){
		LOG << ep.IP() << " " << ep.port();
		Connecting.insert(ep);
		Hash.insert(std::make_pair(ep, hash));
		Proactor.connect(ep);
	}else if(connected){
		//download file from connection
		for(std::map<int, connection_element>::iterator it_cur = Connection.begin(),
			it_end = Connection.end(); it_cur != it_end; ++it_cur)
		{
			if(it_cur->second.ep == ep){
				it_cur->second.Connection->add(hash);
				trigger_tick(it_cur->first);
				break;
			}
		}
	}
}

unsigned connection_manager::connections()
{
	boost::mutex::scoped_lock lock(Connect_mutex);
	return Connection.size();
}

void connection_manager::connect_call_back(net::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connect_mutex);

//DEBUG, disable blacklist
db::pool::singleton()->get()->query("DELETE FROM blacklist");

	if(CI.direction == net::outgoing){
		Connecting.erase(CI.ep);
	}
	if(Connected.find(CI.ep) == Connected.end()){
		Connected.insert(CI.ep);
		LOG << "connect: " << CI.ep.IP() << " " << CI.ep.port();
		boost::shared_ptr<connection> C(new connection(Proactor, CI,
			boost::bind(&connection_manager::peer_call_back, this, _1, _2),
			boost::bind(&connection_manager::trigger_tick, this, _1)));

		//download files from connection
		typedef std::multimap<net::endpoint, std::string>::iterator it_t;
		std::pair<it_t, it_t> p = Hash.equal_range(CI.ep);
		for(; p.first != p.second; ++p.first){
			C->add(p.first->second);
		}

		Connection.insert(std::make_pair(CI.connection_ID, connection_element(CI.ep, C)));
		trigger_tick(CI.connection_ID);
	}else{
		LOG << "dupe connect: " << CI.ep.IP() << " " << CI.ep.port();
		Proactor.disconnect(CI.connection_ID);
	}
}

unsigned connection_manager::DHT_count()
{
	return DHT.count();
}

void connection_manager::disconnect_call_back(net::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connect_mutex);
	Connection.erase(CI.connection_ID);
	if(CI.direction == net::outgoing){
		if(Connecting.erase(CI.ep)){
			//connection failed
			LOG << "connect failed: " << CI.ep.IP() << " " << CI.ep.port();
			Hash.erase(CI.ep);
		}
	}else{
		LOG << "\"" << CI.ep.IP() << "\" " << CI.ep.port();
	}
}

void connection_manager::peer_call_back(const net::endpoint & ep,
	const std::string & hash)
{
	Thread_Pool.enqueue(boost::bind(&connection_manager::connect, this, ep, hash));
}

void connection_manager::remove(const std::string & hash)
{
	Thread_Pool.enqueue(boost::bind(&connection_manager::remove_priv, this, hash));
}

void connection_manager::remove_priv(const std::string hash)
{
	boost::mutex::scoped_lock lock(Connect_mutex);
	share::slot_iterator S_iter = share::singleton()->remove_slot(hash);
	if(S_iter != share::singleton()->end_slot()){
		if(S_iter->get_transfer() && S_iter->get_transfer()->complete()){
			/*
			Don't allow complete files to be removed. We have this check in case
			we somehow get passed a hash for a file that exists in the share. We
			don't want to delete files out of the share, only downloading files.
			*/
			return;
		}
		/*
		After we do share::remove_slot() there aren't any files in the share with
		the hash we removed. We know this because we don't download files we
		already have. Because of this we don't have to worry about a proactor
		thread recreating the slot for a different file with the same hash.
		*/
		for(std::map<int, connection_element>::iterator
			it_cur = Connection.begin(); it_cur != Connection.end(); ++it_cur)
		{
			it_cur->second.Connection->remove(hash);
			if(it_cur->second.Connection->empty()){
				Proactor.disconnect(it_cur->first);
			}
		}
		/*
		At this point we know the slot has been removed from all connections and we
		can proceed to remove the database entry and file (because we know that no
		one is using the file).
		*/
		db::table::share::remove(S_iter->path());
		boost::filesystem::remove(S_iter->path());
	}
}

void connection_manager::set_max_connections(const unsigned incoming_limit,
	const unsigned outgoing_limit)
{
	assert(incoming_limit + outgoing_limit <= 1000);
	Proactor.set_connection_limit(incoming_limit, outgoing_limit);
}

void connection_manager::set_max_download_rate(const unsigned rate)
{
	Proactor.set_max_download_rate(rate);
}

void connection_manager::set_max_upload_rate(const unsigned rate)
{
	Proactor.set_max_upload_rate(rate);
}

void connection_manager::start()
{
	std::set<net::endpoint> E = net::get_endpoint("", db::table::prefs::get_port());
	if(E.empty()){
		LOG << "failed to resolve listener";
		exit(1);
	}
	boost::shared_ptr<net::listener> Listener(new net::listener(*E.begin()));
	if(!Listener->is_open()){
		LOG << "failed to open listener";
		exit(1);
	}
	Proactor.start(Listener);
}

void connection_manager::store_file(const std::string & hash)
{
	DHT.store_file(hash);
}

void connection_manager::tick(const int connection_ID)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(tick_memoize_mutex);
	tick_memoize.erase(connection_ID);
	}//end lock scope
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(Connect_mutex);
	std::map<int, connection_element>::iterator iter = Connection.find(connection_ID);
	if(iter != Connection.end()){
		iter->second.Connection->tick();
	}
	}//END lock scope
}

void connection_manager::trigger_tick(const int connection_ID)
{
	boost::mutex::scoped_lock lock(tick_memoize_mutex);
	std::pair<std::set<int>::iterator, bool> P = tick_memoize.insert(connection_ID);
	if(P.second){
		Thread_Pool.enqueue(boost::bind(&connection_manager::tick, this, connection_ID));
	}
}

unsigned connection_manager::TCP_download_rate()
{
	return Proactor.download_rate();
}

unsigned connection_manager::TCP_upload_rate()
{
	return Proactor.upload_rate();
}

unsigned connection_manager::UDP_download_rate()
{
	return DHT.download_rate();
}

unsigned connection_manager::UDP_upload_rate()
{
	return DHT.upload_rate();
}
