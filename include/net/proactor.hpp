#ifndef H_NET_PROACTOR
#define H_NET_PROACTOR

//custom
#include "buffer.hpp"
#include "protocol.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <thread_pool.hpp>

//standard
#include <map>

namespace net{

//predecl for PIMPL
class proactor_impl;

class proactor : private boost::noncopyable
{
public:
	//info passed to call backs
	class connection_info : private boost::noncopyable
	{
	public:
		connection_info(
			const int connection_ID_in,
			const std::string & host_in,
			const std::string & IP_in,
			const std::string & port_in,
			const direction_t direction_in
		);

		const int connection_ID;     //unique identifier for connection
		const std::string host;      //unresolved host name
		const std::string IP;        //remote IP
		const std::string port;      //remote port
		const direction_t direction; //incoming (remote host initiated connection) or outgoing

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

	proactor(
		const boost::function<void (connection_info &)> & connect_call_back,
		const boost::function<void (connection_info &)> & disconnect_call_back
	);

	/* Stop/Start
	start:
		Start the proactor and optionally add a listener.
		Note: The listener added should not be used after it is passed to the
			proactor.
	stop:
		Stop the proactor. Disconnect call back done for all connections.
		Pending resolve jobs are cancelled.
	*/
	void start(boost::shared_ptr<listener> Listener_in = boost::shared_ptr<listener>());
	void stop();

	/* Connect/Disconnect/Send
	connect:
		Start async connection to host. Connect call back will happen if connects,
		or disconnect call back if connection fails.
	disconnect:
		Disconnect as soon as possible.
	disconnect_on_empty:
		Disconnect when send buffer becomes empty.
	send:
		Send data to specified connection if connection still exists.
	*/
	void connect(const std::string & host, const std::string & port);
	void disconnect(const int connection_ID);
	void disconnect_on_empty(const int connection_ID);
	void send(const int connection_ID, buffer & send_buf);


	/* Info
	connections:
		Returns how many connections are open. Includes half open connections.
	download_rate:
		Returns download rate averaged over a few seconds (B/s).
	listen_port:
		Returns port listening on (empty if not listening).
	upload_rate:
		Returns upload rate averaged over a few seconds (B/s).
	*/
	unsigned connections();
	unsigned download_rate();
	std::string listen_port();
	unsigned upload_rate();

	/* Get Options
	get_max_download_rate:
		Get maximum allowed download rate.
	get_max_upload_rate:
		Get maximum allowed upload rate.
	*/
	unsigned get_max_download_rate();
	unsigned get_max_upload_rate();

	/* Set Options
	set_connection_limit:
		Set connection limit for incoming and outgoing connections.
		Note: incoming_limit + outgoing_limit <= 1024.
	set_max_download_rate:
		Set maximum allowed download rate.
	set_max_upload_rate:
		Set maximum allowed upload rate.
	*/
	void set_connection_limit(const unsigned incoming_limit, const unsigned outgoing_limit);
	void set_max_download_rate(const unsigned rate);
	void set_max_upload_rate(const unsigned rate);

private:
	boost::shared_ptr<proactor_impl> Proactor_impl;
};
}//end namespace net
#endif
