#ifndef H_NETWORK
#define H_NETWORK

//custom
#include "reactor_select.hpp"
#include "validate.hpp"

//include
#include <boost/shared_ptr.hpp>

namespace network{
typedef socket_data_visible socket;
class io_service
{
public:
	/* ctor parameters
	boost::bind is used to do a callback to the member function of a
	specific object. ex:
		boost::bind(&http::connect_call_back, &HTTP, _1, _2, _3)
	*/
	io_service(
		boost::function<void (const std::string & host, const std::string & port,
			const network::ERROR error)> failed_connect_call_back_in,
		boost::function<void (socket_data_visible & Socket)> connect_call_back_in,
		boost::function<void (socket_data_visible & Socket)> disconnect_call_back_in,
		const unsigned max_incoming_connections,
		const unsigned max_outgoing_connections,
		const std::string & port = "-1"
	):
		Reactor(new reactor_select(
			failed_connect_call_back_in,
			connect_call_back_in,
			disconnect_call_back_in,
			port
		))
	{
		/*
		Connection limits must be set before reactor started otherwise incoming
		connections might be rejected.
		*/
		set_max_connections(max_incoming_connections, max_outgoing_connections);

		Reactor->start();
	}

	~io_service()
	{
		Reactor->stop();
	}

	/*
	Asynchronously connect to a host. This function does NOT block. The host can
	be a IP address or hostname. DNS resolution done with an internal thread
	pool.
	*/
	bool connect(const std::string & host, const std::string & port)
	{
		if(valid_address(host) && valid_port(port)){
			Reactor->connect(host, port);
			return true;
		}else{
			return false;
		}
	}

	/* Connection count/limit related functions.
	connections:
		Returns number of connections (both connected and connecting).
	incoming_connections:
		Returns number of connections that remote hosts established with us.
	max_incoming_connections:
		Returns maximum allowed number of incoming connections.
	max_outgoing_connections:
		Returns maximum allowed number of outgoing connections.
	max_connections_supported:
		Returns the maximum number of connections supported by the reactor. This
		should be used when trying to determine the maximum incoming/outgoing
		connections limits.
	outgoing_connections:
		Returns number of connections that we established with remote hosts.
	set_max_connections:
		Sets the maximum number of incoming and outgoing connections.
		Note: The sum of the max incoming/outgoing must not exceed the max
			connections supported. Use max_connections_supported() to determine how
			many connections are supported.
	*/
	unsigned connections()
	{
		return Reactor->get_connections();
	}
	unsigned incoming_connections()
	{
		return Reactor->get_incoming_connections();
	}
	unsigned max_incoming_connections()
	{
		return Reactor->get_max_incoming_connections();
	}
	unsigned max_outgoing_connections()
	{
		return Reactor->get_max_outgoing_connections();
	}
	static unsigned max_connections_supported()
	{
		return reactor_select::max_connections_supported();
	}
	unsigned outgoing_connections()
	{
		return Reactor->get_outgoing_connections();
	}
	void set_max_connections(const unsigned max_incoming_connections,
		const unsigned max_outgoing_connections)
	{
		if(max_incoming_connections + max_outgoing_connections > max_connections_supported()){
			LOGGER << "attempted to set max connections beyond supported";
			exit(1);
		}
		Reactor->set_max_connections(max_incoming_connections, max_outgoing_connections);
	}

	/* Rate related functions.
	current_download_rate:
		Returns current download rate (bytes/second).
	current_upload_rate:
		Returns current upload rate (bytes/second).
	get_max_download_rate:
		Returns current limit on download rate (bytes/second).
	get_max_upload_rate:
		Returns current limit on upload rate (bytes/second).
	set_max_download_rate:
		Sets the maximum download rate (bytes/second).
	set_max_upload_rate:
		Sets the maximum upload rate (bytes/second).
	*/
	unsigned current_download_rate()
	{
		return Reactor->current_download_rate();
	}
	unsigned current_upload_rate()
	{
		return Reactor->current_upload_rate();
	}
	unsigned get_max_download_rate()
	{
		return Reactor->get_max_download_rate();
	}
	unsigned get_max_upload_rate()
	{
		return Reactor->get_max_upload_rate();
	}
	void set_max_download_rate(const unsigned rate)
	{
		Reactor->set_max_download_rate(rate);
	}
	void set_max_upload_rate(const unsigned rate)
	{
		Reactor->set_max_upload_rate(rate);
	}

private:
	boost::shared_ptr<reactor> Reactor;
};
}//end network namespace
#endif
