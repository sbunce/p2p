#include "connection_manager.hpp"

connection_manager::connection_manager():
	Proactor(
		boost::bind(&connection_manager::connect_call_back, this, _1),
		boost::bind(&connection_manager::disconnect_call_back, this, _1)
	)
{

}

connection_manager::~connection_manager()
{
	Proactor.stop();
}

void connection_manager::connect_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);

//DEBUG, disable persistent blacklist
	database::pool::get()->query("DELETE FROM blacklist");

	LOGGER << "connect " << CI.IP << " " << CI.port;
	std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
		ret = Connection.insert(std::make_pair(CI.connection_ID,
		new connection(Proactor, CI)));
	assert(ret.second);
}

void connection_manager::disconnect_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	LOGGER << "disconnect " << CI.IP << " " << CI.port;
	Connection.erase(CI.connection_ID);
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
		}
		/*
		At this point we know the slot has been removed from all connections and we
		can proceed to remove the database entry and file (because we know that no
		one is using the file).
		*/
		database::table::share::remove(S_iter->path());
		boost::filesystem::remove(S_iter->path());
	}
}
