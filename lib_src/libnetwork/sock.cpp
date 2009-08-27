#include <network/network.hpp>

//used for PIMPL
#include "wrapper.hpp"

network::sock::sock(const int socket_FD_in):
	info(new address_info()),
	socket_FD(socket_FD_in),
	IP(wrapper::get_IP(socket_FD)),
	port(wrapper::get_port(socket_FD)),
	last_active(std::time(NULL)),
	direction(INCOMING),
	connect_flag(false),
	disconnect_flag(false),
	failed_connect_flag(false),
	recv_flag(false),
	send_flag(false),
	sock_error(NO_ERROR),
	timeout(DEFAULT_TIMEOUT)
{

}

network::sock::sock(boost::shared_ptr<address_info> info_in):
	info(info_in),
	socket_FD(-1),
	host(info->get_host()),
	IP(wrapper::get_IP(*info)),
	port(info->get_port()),
	last_active(std::time(NULL)),
	direction(OUTGOING),
	connect_flag(false),
	disconnect_flag(false),
	failed_connect_flag(false),
	recv_flag(false),
	send_flag(false),
	sock_error(NO_ERROR),
	timeout(DEFAULT_TIMEOUT)
{

}

network::sock::~sock()
{
	if(socket_FD != -1){
		close(socket_FD);
	}
}

void network::sock::seen()
{
	last_active = std::time(NULL);
}

//returns true if the socket has timed out
bool network::sock::timed_out()
{
	return std::time(NULL) - last_active > timeout;
}
