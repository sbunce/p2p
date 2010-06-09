#ifndef H_PEER
#define H_PEER

//include
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <net/net.hpp>
#include <p2p.hpp>

//standard
#include <queue>
#include <set>

/*
Keeps track of what peer_* messages need to be send. Also, download/upload
speeds.
*/
class peer
{
public:
	/* Register/Unregister Peer
	These are called when we open a slot with a remote host, or when a remote
	host opens a slot with us.
	*/
	void reg(const int connection_ID, const net::endpoint & ep);
	void unreg(const int connection_ID);

	/*
	download_speed_calc:
		Returns download speed_calc for specific peer or empty shared_ptr.
	get:
		Get endpoint which needs to be sent in a peer_* message. The connection_ID
		is the connection we want to send a peer_* message to.
	host_info:
		Returns info for all connected hosts.
	upload_speed_calc:
		Returns upload speed_calc for specific peer or empty shared_ptr.
	*/
	boost::shared_ptr<net::speed_calc> download_speed_calc(const int connection_ID);
	boost::optional<net::endpoint> get(const int connection_ID);
	std::list<p2p::transfer_info::host_element> host_info();
	boost::shared_ptr<net::speed_calc> upload_speed_calc(const int connection_ID);

private:
	//peer object set up as monitor
	boost::mutex Mutex;

	class conn_element
	{
	public:
		conn_element(const net::endpoint & remote_ep_in);
		conn_element(const conn_element & CE);

		const net::endpoint remote_ep;
		boost::shared_ptr<net::speed_calc> Download_Speed;
		boost::shared_ptr<net::speed_calc> Upload_Speed;

		//incremented on reg, decremented on unred, conn_element removed if 0
		unsigned reg_cnt;

		/*
		add:
			Add peer to be sent in peer_* message.
			Note: The endpoint won't be added if ep == remote_ep, or if ep was
				already added.
		get:
			Get enpoint which needs to be sent in peer_* message.
		*/
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
