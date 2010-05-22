#include "connection_manager.hpp"

//BEGIN connection_element
connection_manager::connection_element::connection_element(
	const std::string & IP_in,
	const std::string & port_in,
	const boost::shared_ptr<connection> & Connection_in
):
	IP(IP_in),
	port(port_in),
	Connection(Connection_in)
{

}

connection_manager::connection_element::connection_element(const connection_element & CE):
	IP(CE.IP),
	port(CE.port),
	Connection(CE.Connection)
{

}
//END connection_element

connection_manager::connection_manager():
	Proactor(
		boost::bind(&connection_manager::connect_call_back, this, _1),
		boost::bind(&connection_manager::disconnect_call_back, this, _1)
	),
	Thread_Pool(1),
	_connections(0)
{

}

connection_manager::~connection_manager()
{
	DHT.stop();
	Proactor.stop();
	Thread_Pool.stop_join();
}

void connection_manager::add(const std::string & hash)
{
	DHT.find_file(hash, boost::bind(&connection_manager::add_call_back, this, _1));
}

void connection_manager::connection_manager::add_call_back(const net::endpoint & ep)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	if(Connection_Hosts.find(std::make_pair(ep.IP(), ep.port())) == Connection_Hosts.end()){
		Connection_Hosts.insert(std::make_pair(ep.IP(), ep.port()));
		LOG << ep.IP() << " " << ep.port();
		Proactor.connect(ep.IP(), ep.port());
	}
}

unsigned connection_manager::connections()
{
	return _connections;
}

void connection_manager::connect_call_back(net::proactor::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	bool is_connected = false;
	for(std::map<int, connection_element>::iterator it_cur = Connection.begin(),
		it_end = Connection.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second.IP == CI.IP && it_cur->second.port == CI.port){
			is_connected = true;
			break;
		}
	}
	if(is_connected){
		LOG << "dupe connect: " << CI.IP << " " << CI.port;
		Proactor.disconnect(CI.connection_ID);
	}else{
		LOG << "connect: " << CI.IP << " " << CI.port;
		boost::shared_ptr<connection> C(new connection(Proactor, CI,
			boost::bind(&connection_manager::trigger_tick, this, _1)));
		std::pair<std::map<int, connection_element>::iterator, bool>
			ret = Connection.insert(std::make_pair(CI.connection_ID,
			connection_element(CI.IP, CI.port, C)));
		assert(ret.second);
		++_connections;
	}
}

unsigned connection_manager::DHT_count()
{
	return DHT.count();
}

void connection_manager::disconnect_call_back(net::proactor::connection_info & CI)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(Connection_mutex);
	LOG << "\"" << (!CI.host.empty() ? CI.host : CI.IP) << "\" " << CI.port;
	Connection.erase(CI.connection_ID);
	//duplicate connections don't have a Connection element
	if(Connection.find(CI.connection_ID) != Connection.end()){
		Connection_Hosts.erase(std::make_pair(CI.IP, CI.port));
	}
	}//END lock scope
	--_connections;
}

void connection_manager::remove(const std::string & hash)
{
	Thread_Pool.enqueue(boost::bind(&connection_manager::remove_priv, this, hash));
}

void connection_manager::remove_priv(const std::string hash)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	share::slot_iterator S_iter = share::singleton().remove_slot(hash);
	if(S_iter != share::singleton().end_slot()){
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
	boost::mutex::scoped_lock lock(Connection_mutex);
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
