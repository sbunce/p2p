#ifndef H_NETWORK
#define H_NETWORK

//boost
#include <boost/shared_ptr.hpp>

/*
Select which reactor to use on each OS. These must correspond with the
instantiated reactor in the network::io_service ctor.
*/
#ifdef LINUX2
	#include "reactor_select.hpp"
#elif WIN32
	#include "reactor_select.hpp"
#endif

namespace network{
typedef socket_data::socket_data_visible socket;
class io_service
{
public:
	/* ctor parameters
	boost::bind is used to do a callback to the member function of a
	specific object. ex:
		boost::bind(&http::connect_call_back, &HTTP, _1, _2, _3)
	*/
	io_service(
		boost::function<void (const std::string & host, const std::string & port)> failed_connect_call_back_in,
		boost::function<void (socket_data::socket_data_visible & Socket)> connect_call_back_in,
		boost::function<void (socket_data::socket_data_visible & Socket)> disconnect_call_back_in,
		const std::string & port = "-1"
	):
		Reactor(new
		#ifdef LINUX2
		reactor_select(
		#elif WIN32
		reactor_select(
		#endif
			failed_connect_call_back_in,
			connect_call_back_in,
			disconnect_call_back_in,
			port
		))
	{
		Reactor->start();
	}

	~io_service()
	{
		Reactor->stop();
	}

	//asynchronously connect to host
	void connect(const std::string & host, const std::string & port)
	{
		Reactor->connect(host, port);
	}

	//returns how many connections are established
	unsigned connections()
	{
		return Reactor->get_connections();
	}
	unsigned incoming_connections()
	{
		return Reactor->get_incoming_connections();
	}
	unsigned outgoing_connections()
	{
		return Reactor->get_outgoing_connections();
	}

	/*
	Functions for getting/setting rate limits (bytes), and getting current rates
	(bytes/second).
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
