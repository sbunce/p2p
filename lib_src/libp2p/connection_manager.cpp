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
//DEBUG, disable persistent blacklist
database::pool::get()->query("DELETE FROM blacklist");
//DEBUG, need to check blacklist here

	{//begin lock scope
	boost::mutex::scoped_lock lock(Connection_mutex);
	for(std::map<int, boost::shared_ptr<connection> >::iterator
		iter_cur = Connection.begin(), iter_end = Connection.end();
		iter_cur != iter_end; ++iter_cur)
	{
		if(CI.IP == iter_cur->second->IP){
			LOGGER << "duplicate connect " << CI.IP << " " << CI.port;
			Proactor.disconnect(CI.connection_ID);
			return;
		}
	}
	LOGGER << "connect " << CI.IP << " " << CI.port;
	std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
		ret = Connection.insert(std::make_pair(CI.connection_ID,
		new connection(Proactor, CI)));
	assert(ret.second);
	}//end lock scope
}

void connection_manager::disconnect_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	LOGGER << "disconnect " << CI.IP << " " << CI.port;
	Connection.erase(CI.connection_ID);
}
