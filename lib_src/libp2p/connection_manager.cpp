#include "connection_manager.hpp"

connection_manager::connection_manager(
	share & Share_in
):
	Share(Share_in)
{

}

/*
void connection_manager::connect_call_back(network::sock & S)
{
	LOGGER << "connect " << S.IP << " " << S.port;
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
		ret = Connection.insert(std::make_pair(S.socket_FD, new connection(S, Share)));
	assert(ret.second);
}

void connection_manager::disconnect_call_back(network::sock & S)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	LOGGER << "disconnect " << S.IP << " " << S.port;
	if(Connection.erase(S.socket_FD) != 1){
		LOGGER << "disconnect called for socket that never connected";
		exit(1);
	}
}

void connection_manager::failed_connect_call_back(network::sock & S)
{
	LOGGER << "failed connect " << S.IP << " " << S.port;
}
*/
