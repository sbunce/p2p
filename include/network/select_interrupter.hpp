#ifndef H_NETWORK_SELECT_INTERRUPTER
#define H_NETWORK_SELECT_INTERRUPTER

//custom
#include "endpoint.hpp"

//include
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//standard
#include <sstream>

namespace network{
/*
The select interrupter is used for the "selfpipe trick". A pair of connected
sockets is created. The receiving socket is put in the fd_set that select()
monitors. The sending socket is used to trigger select to return.
*/
class select_interrupter : private boost::noncopyable
{
public:
	select_interrupter()
	{
		network::start();

		/*
		Create socket pair, one side writes the other side reads. The select()
		function will monitor the read_only socket.
		*/
		//setup listener
		std::set<network::endpoint> E = network::get_endpoint("localhost", "0", network::tcp);
		assert(!E.empty());
		network::listener L(*E.begin());
		assert(L.is_open());

		//connect read_only socket
		E = network::get_endpoint("localhost", L.port(), network::tcp);
		assert(!E.empty());
		read_only = boost::shared_ptr<nstream>(new nstream(*E.begin()));
		assert(read_only->is_open());

		//accept write_only connection
		write_only = L.accept();
		assert(write_only);
	}

	~select_interrupter()
	{
		network::stop();
	}

	/*
	Triggers select to return. May be called by any thread.
	*/
	void trigger()
	{
		boost::mutex::scoped_lock lock(write_only_mutex);
		buffer B("0");
		write_only->send(B);
	}

	/*
	Returns the socket select needs to monitor.
	Note: This needs to be added to the read fd_set that select() monitors.
	*/
	int socket()
	{
		return read_only->socket();
	}

	/*
	Must be called if socket is readable.
	Note: This must only be called from the networking thread.
	*/
	void reset()
	{
		//discard any incoming bytes
		buffer B;
		read_only->recv(B);
	}

private:
	//read by select thread only, no need to lock
	boost::shared_ptr<nstream> read_only;

	/*
	The notify() function is called by multiple threads so the write_only nstream
	must be locked.
	*/
	boost::mutex write_only_mutex;
	boost::shared_ptr<nstream> write_only;
};
}
#endif
