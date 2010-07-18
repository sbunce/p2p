#ifndef H_NET_CONNECTION_INFO
#define H_NET_CONNECTION_INFO

//custom
#include "buffer.hpp"
#include "endpoint.hpp"
#include "protocol.hpp"

//standard
#include <ctime>
#include <set>

namespace net{
//info passed to proactor call backs
class connection_info : private boost::noncopyable
{
public:
	connection_info(
		const int connection_ID_in,
		const endpoint & ep_in,
		const direction_t direction_in
	);

	const int connection_ID;
	const endpoint ep;
	const direction_t direction;

	/*
	The recv_call_back must be set in the connect call back to recieve incoming
	data. If the recv_call_back is not set during the connect call back then
	incoming data will be discarded.
	*/
	boost::function<void (connection_info &)> recv_call_back;
	boost::function<void (connection_info &)> send_call_back;

	/*
	recv_buf:
		Received data appended to this buffer.
	latest_recv:
		How much data was appended last.
	latest_send:
		Size of the latest send. This value will be stale during all but
		send_call_back.
	*/
	buffer recv_buf;
	unsigned latest_recv;
	unsigned latest_send;

	/*
	The size of the send_buf only accessible to the proactor. This value can be
	really old if checked during the recv_call_back. This value is most up to
	date when checked during the send_call_back. If the send_call_back is set
	then a call back will happen whenever this value decreases.
	*/
	unsigned send_buf_size;
};
}//end namespace net
#endif
