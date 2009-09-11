#include "connection_manager.hpp"

connection_manager::connection_manager()
{

}

void connection_manager::connect_call_back(network::sock & S)
{
	LOGGER << "connect " << S.IP;
	boost::mutex::scoped_lock lock(Connection_mutex);
	std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
		ret = Connection.insert(std::make_pair(S.socket_FD, new connection(S)));
	assert(ret.second);
	S.recv_call_back = boost::bind(&connection::recv_call_back, ret.first->second.get(), _1);
	S.send_call_back = boost::bind(&connection::send_call_back, ret.first->second.get(), _1);
}

void connection_manager::disconnect_call_back(network::sock & S)
{
	boost::mutex::scoped_lock lock(Connection_mutex);
	if(Connection.erase(S.socket_FD) != 1){
		LOGGER << "disconnect called for socket that never connected";
		exit(1);
	}
}

void connection_manager::failed_connect_call_back(network::sock & S)
{
	LOGGER << "failed connect to " << S.socket_FD;
}
