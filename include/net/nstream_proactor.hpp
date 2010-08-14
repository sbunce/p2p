#ifndef H_NET_NSTREAM_PROACTOR
#define H_NET_NSTREAM_PROACTOR

//custom
#include "buffer.hpp"
#include "init.hpp"
#include "listener.hpp"
#include "nstream.hpp"
#include "rate_limit.hpp"
#include "select.hpp"

//include
#include <boost/shared_ptr.hpp>
//#include <channel.hpp>
#include <portable.hpp>

//standard
#include <map>
#include <queue>

namespace net{
class nstream_proactor : private boost::noncopyable
{
	net::init Init;
public:

	enum dir_t{
		incoming, //remote host initiated connection
		outgoing  //we initiated connection
	};

	class connect_event
	{
	public:
		boost::uint64_t conn_ID;
		dir_t dir;
		endpoint ep;
	};

	class disconnect_event
	{
	public:
		boost::uint64_t conn_ID;
	};

	class recv_event
	{
	public:
		boost::uint64_t conn_ID;
		buffer buf;
	};

	class send_event
	{
	public:
		boost::uint64_t conn_ID;
		unsigned send_buf_size;	
	};

	nstream_proactor(
		const boost::function<void (connect_event)> & connect_call_back_in,
		const boost::function<void (disconnect_event)> & disconnect_call_back_in,
		const boost::function<void (recv_event)> & recv_call_back_in,
		const boost::function<void (send_event)> & send_call_back_in
	):
		unused_conn_ID(0)
	{

	}

private:
	class conn : private boost::noncopyable
	{
	public:
		/*
		read:
			Called when select reports socket is ready to read.
		timeout:
			Connection must be removed if this returns true.
		write:
			Called when select reports socket is ready to write.
		*/
		virtual void read() = 0;
		virtual bool timeout() = 0;
		virtual void write(){};
	};

	class conn_nstream : public conn
	{
	public:

	private:

	};

	class conn_listener : public conn
	{
	public:
		conn_listener(
			boost::shared_ptr<listener> Listener_in,
			std::set<int> & read_FDS_in
		):
			Listener(Listener_in),
			read_FDS(read_FDS_in)
		{
			assert(Listener);
		}

		virtual void read()
		{
			while(boost::shared_ptr<nstream> N = Listener->accept()){
LOG << "incoming connection received.";
			}
		}

		virtual bool timeout()
		{
			//listener never times out
			return false;
		}

	private:
		boost::shared_ptr<listener> Listener;
		std::set<int> & read_FDS;
		//std::map<int, boost::shared_ptr<conn> > & Socket;
		//std::map<int, boost::shared_ptr<conn> > & ID;
	};

	boost::uint64_t unused_conn_ID;                 //new conn_ID's are unused_conn_ID++
	select Select;                                  //wrapper for BSD sockets select()
	std::set<int> read_FDS;                         //sockets for select to monitor for read
	std::set<int> write_FDS;                        //sockets for select to monitor for write
	std::map<int, boost::shared_ptr<conn> > Socket; //socket_FD associated with conn
	std::map<int, boost::shared_ptr<conn> > ID;     //conn_ID associated with conn

	void main_loop()
	{

	}

	/*
	Thread pool is used to run main_loop. It is also used by public functions to
	schedule calls of private functions. This insures private data members are
	only modified by one thread.
	*/
};
}//end namespace net
#endif
