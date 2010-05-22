#include "connection_manager.hpp"

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
	Proactor.stop();
	Thread_Pool.stop_join();
}

void connection_manager::connect_call_back(net::proactor::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	LOG << "connect " << CI.IP << " " << CI.port;
	std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
		ret = Connection.insert(std::make_pair(CI.connection_ID,
		new connection(Proactor, CI, boost::bind(&connection_manager::trigger_tick,
		this, _1))));
	assert(ret.second);
}

unsigned connection_manager::DHT_count()
{
	return DHT.count();
}

void connection_manager::disconnect_call_back(net::proactor::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	LOG << "disconnect \"" << (!CI.host.empty() ? CI.host : CI.IP) << "\" " << CI.port;
	Connection.erase(CI.connection_ID);
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
		for(std::map<int, boost::shared_ptr<connection> >::iterator
			it_cur = Connection.begin(); it_cur != Connection.end(); ++it_cur)
		{
			it_cur->second->remove(hash);
			if(it_cur->second->empty()){
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
	std::set<net::endpoint> E = net::get_endpoint(
		"",
		db::table::prefs::get_port()
	);
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

void connection_manager::tick(const int connection_ID)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(filter_mutex);
	filter.erase(connection_ID);
	}//end lock scope
	{//BEGIN lock scope

	boost::mutex::scoped_lock lock(Connection_mutex);
	std::map<int, boost::shared_ptr<connection> >::iterator
		iter = Connection.find(connection_ID);
	if(iter != Connection.end()){
		iter->second->tick();
	}
	}//END lock scope
}

void connection_manager::trigger_tick(const int connection_ID)
{
	boost::mutex::scoped_lock lock(filter_mutex);
	std::pair<std::set<int>::iterator, bool> P = filter.insert(connection_ID);
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
