#include <net/connection_info.hpp>

net::connection_info::connection_info(
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
