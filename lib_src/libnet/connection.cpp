#include "connection.hpp"

net::connection::connection(
	ID_manager & ID_Manager_in,
	const std::string & host_in,
	const std::string & port_in
):
	connected(false),
	ID_Manager(ID_Manager_in),
	last_seen(std::time(NULL)),
	disc_on_empty(false),
	N(new nstream()),
	host(host_in),
	port(port_in),
	connection_ID(ID_Manager.allocate())
{
	E = get_endpoint(host, port);
	if(E.empty()){
		CI.reset(new proactor::connection_info(connection_ID, host, "", port, incoming));
	}
}

net::connection::connection(
	ID_manager & ID_Manager_in,
	const boost::shared_ptr<nstream> & N_in
):
	connected(true),
	ID_Manager(ID_Manager_in),
	last_seen(std::time(NULL)),
	socket_FD(N_in->socket()),
	disc_on_empty(false),
	N(N_in),
	host(""),
	port(N_in->remote_port()),
	connection_ID(ID_Manager.allocate())
{
	CI.reset(new proactor::connection_info(connection_ID, "", N->remote_IP(), N->remote_port(), incoming));
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

bool net::connection::open_async()
{
	//assert this connection is outgoing
	assert(!host.empty());
	if(E.empty()){
		//no more endpoints to try
		return false;
	}
	N->open_async(*E.begin());
	socket_FD = N->socket();
	CI.reset(new proactor::connection_info(connection_ID, host, E.begin()->IP(), port, outgoing));
	E.erase(E.begin());
	return socket_FD != -1;
}

int net::connection::socket()
{
	return socket_FD;
}

bool net::connection::timed_out()
{
	return std::time(NULL) - last_seen > idle_timeout;
}

void net::connection::touch()
{
	last_seen = std::time(NULL);
}
