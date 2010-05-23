#include "connection.hpp"

net::connection::connection(
	ID_manager & ID_Manager_in,
	const endpoint & ep
):
	connected(false),
	ID_Manager(ID_Manager_in),
	time(std::time(NULL) + idle_timeout),
	disc_on_empty(false),
	N(new nstream()),
	connection_ID(ID_Manager.allocate())
{
	N->open_async(ep);
	socket_FD = N->socket();
	CI.reset(new proactor::connection_info(connection_ID, ep, outgoing));
}

net::connection::connection(
	ID_manager & ID_Manager_in,
	const boost::shared_ptr<nstream> & N_in
):
	connected(true),
	ID_Manager(ID_Manager_in),
	time(std::time(NULL) + idle_timeout),
	socket_FD(N_in->socket()),
	disc_on_empty(false),
	N(N_in),
	connection_ID(ID_Manager.allocate())
{
	std::set<endpoint> E = get_endpoint(N->remote_IP(), N->remote_port());
	assert(!E.empty());
	CI.reset(new proactor::connection_info(connection_ID, *E.begin(), incoming));
}

net::connection::~connection()
{
	ID_Manager.deallocate(connection_ID);
}

bool net::connection::half_open()
{
	return !connected;
}

bool net::connection::is_open()
{
	if(N->is_open_async()){
		connected = true;
		return true;
	}
	return false;
}

int net::connection::socket()
{
	return socket_FD;
}

bool net::connection::timed_out()
{
	return std::time(NULL) > time;
}

void net::connection::touch()
{
	time = std::time(NULL) + idle_timeout;
}
