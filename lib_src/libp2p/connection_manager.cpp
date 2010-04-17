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
	Thread_Pool.stop();
}

void connection_manager::connect_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);

//DEBUG, disable persistent blacklist
	db::pool::get()->query("DELETE FROM blacklist");

	LOG << "connect " << CI.IP << " " << CI.port;
	std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
		ret = Connection.insert(std::make_pair(CI.connection_ID,
		new connection(Proactor, CI, boost::bind(&connection_manager::trigger_tick,
		this, _1))));
	assert(ret.second);
}

void connection_manager::disconnect_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	LOG << "disconnect \"" << (!CI.host.empty() ? CI.host : CI.IP) << "\" " << CI.port;
	Connection.erase(CI.connection_ID);
}

void connection_manager::do_tick(const int connection_ID)
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

void connection_manager::remove(const std::string hash)
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
			iter_cur = Connection.begin(); iter_cur != Connection.end(); ++iter_cur)
		{
			iter_cur->second->remove(hash);
			if(iter_cur->second->empty()){
				Proactor.disconnect(iter_cur->first);
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

void connection_manager::trigger_tick(const int connection_ID)
{
	boost::mutex::scoped_lock lock(filter_mutex);
	//if cannot insert then we know job already scheduled
	std::pair<std::set<int>::iterator, bool> P = filter.insert(connection_ID);
	if(P.second){
		Thread_Pool.enqueue(boost::bind(&connection_manager::do_tick, this, connection_ID));
	}
}
