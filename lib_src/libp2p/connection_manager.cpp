#include "connection_manager.hpp"

connection_manager::connection_manager(
	network::proactor & Proactor_in
):
	Proactor(Proactor_in)
{

}

void connection_manager::connect_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Mutex);
	std::pair<std::set<std::string>::iterator, bool> ret = IP.insert(CI.IP);
	if(ret.second){
		LOGGER << "connect " << CI.IP << " " << CI.port;
		std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
			ret = Connection.insert(std::make_pair(CI.connection_ID,
			new connection(Proactor, CI)));
		assert(ret.second);
	}else{
		LOGGER << "duplicate connect " << CI.IP << " " << CI.port;
		Proactor.disconnect(CI.connection_ID);
	}
}

void connection_manager::disconnect_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Mutex);
	LOGGER << "disconnect " << CI.IP << " " << CI.port;
	IP.erase(CI.IP);
	Connection.erase(CI.connection_ID);
}
