#include "proactor_impl.hpp"
#include <network/proactor.hpp>

//BEGIN connection_info
network::proactor::connection_info::connection_info(
	const int connection_ID_in,
	const std::string & host_in,
	const std::string & IP_in,
	const std::string & port_in,
	const direction_t direction_in
):
	connection_ID(connection_ID_in),
	host(host_in),
	IP(IP_in),
	port(port_in),
	direction(direction_in),
	latest_recv(0),
	latest_send(0),
	send_buf_size(0)
{

}
//END connection_info

network::proactor::proactor(
	const boost::function<void (proactor::connection_info &)> & connect_call_back,
	const boost::function<void (proactor::connection_info &)> & disconnect_call_back
):
	Proactor_impl(new proactor_impl(
		connect_call_back,
		disconnect_call_back
	))
{

}

void network::proactor::connect(const std::string & host, const std::string & port)
{
	Proactor_impl->connect(host, port);
}

void network::proactor::disconnect(const int connection_ID)
{
	Proactor_impl->disconnect(connection_ID);
}

void network::proactor::disconnect_on_empty(const int connection_ID)
{
	Proactor_impl->disconnect_on_empty(connection_ID);
}

unsigned network::proactor::download_rate()
{
	return Proactor_impl->download_rate();
}

unsigned network::proactor::get_max_download_rate()
{
	return Proactor_impl->get_max_download_rate();
}

unsigned network::proactor::get_max_upload_rate()
{
	return Proactor_impl->get_max_upload_rate();
}

std::string network::proactor::listen_port()
{
	return Proactor_impl->listen_port();
}

void network::proactor::start(boost::shared_ptr<listener> Listener_in)
{
	Proactor_impl->start(Listener_in);
}

void network::proactor::stop()
{
	Proactor_impl->stop();
}

void network::proactor::send(const int connection_ID, buffer & send_buf)
{
	Proactor_impl->send(connection_ID, send_buf);
}

void network::proactor::set_connection_limit(const unsigned incoming_limit,
	const unsigned outgoing_limit)
{
	Proactor_impl->set_connection_limit(incoming_limit, outgoing_limit);
}

void network::proactor::set_max_download_rate(const unsigned rate)
{
	Proactor_impl->set_max_download_rate(rate);
}

void network::proactor::set_max_upload_rate(const unsigned rate)
{
	Proactor_impl->set_max_upload_rate(rate);
}

unsigned network::proactor::upload_rate()
{
	return Proactor_impl->upload_rate();
}
