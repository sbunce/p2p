#ifndef H_PEER
#define H_PEER

//include
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <net/net.hpp>

//standard
#include <queue>
#include <set>

class peer
{
public:
	/*
	add:
		Add endpoint that has subscribed (incoming or outgoing).
	get:
		Get endpoint which needs to be sent in a peer_* message.
	*/
	void add(const int connection_ID, const net::endpoint & ep);
	boost::optional<net::endpoint> get(const int connection_ID);

private:
	//peer object set up as monitor
	boost::mutex Mutex;

	class conn_element
	{
	public:
		conn_element(const net::endpoint & remote_ep_in);
		conn_element(const conn_element & CE);

		//endpoint this conn can be reached on
		const net::endpoint remote_ep;

		//same as peer public functions but works on specific connection_ID
		void add(const net::endpoint & ep);
		boost::optional<net::endpoint> get();
	private:
		std::queue<net::endpoint> unsent; //to send in peer_* messages
		std::set<net::endpoint> memoize;  //stops duplicates from being sent
	};

	//connection_ID is the key
	std::map<int, conn_element> Conn;
};
#endif
