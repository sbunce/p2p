#include "proactor_impl.hpp"
#include <net/proactor.hpp>

//BEGIN connection_info
net::proactor::connection_info::connection_info(
	const int connection_ID_in,
	const endpoint & ep_in,
	const direction_t direction_in
):
	connection_ID(connection_ID_in),
	ep(ep_in),
	direction(direction_in),
	latest_recv(0),
	latest_send(0),
	send_buf_size(0)
{

}
//END connection_info

net::proactor::proactor(
	const boost::function<void (proactor::connection_info &)> & connect_call_back,
	const boost::function<void (proactor::connection_info &)> & disconnect_call_back
):
	Proactor_impl(new proactor_impl(
		connect_call_back,
		disconnect_call_back
	))
{

}

void net::proactor::connect(const endpoint & ep)
{
	Proactor_impl->connect(ep);
}

void net::proactor::disconnect(const int connection_ID)
{
	Proactor_impl->disconnect(connection_ID);
}

void net::proactor::disconnect_on_empty(const int connection_ID)
{
	Proactor_impl->disconnect_on_empty(connection_ID);
}

unsigned net::proactor::download_rate()
{
	return Proactor_impl->download_rate();
}

void net::proactor::start(boost::shared_ptr<listener> Listener_in)
{
	Proactor_impl->start(Listener_in);
}

void net::proactor::stop()
{
	Proactor_impl->stop();
}

void net::proactor::send(const int connection_ID, buffer & send_buf)
{
	Proactor_impl->send(connection_ID, send_buf);
}

void net::proactor::set_connection_limit(const unsigned incoming_limit,
	const unsigned outgoing_limit)
{
	Proactor_impl->set_connection_limit(incoming_limit, outgoing_limit);
}

void net::proactor::set_max_download_rate(const unsigned rate)
{
	Proactor_impl->set_max_download_rate(rate);
}

void net::proactor::set_max_upload_rate(const unsigned rate)
{
	Proactor_impl->set_max_upload_rate(rate);
}

unsigned net::proactor::upload_rate()
{
	return Proactor_impl->upload_rate();
}
