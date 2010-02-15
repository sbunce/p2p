#ifndef H_KADEMLIA
#define H_KADEMLIA

//custom
#include "database.hpp"

//include
#include <network/network.hpp>

class kademlia
{
public:
	kademlia();
	~kademlia();

	/*
	add_node:
		Try to add node to k-bucket. Update k-bucket timer if node already exists.
		Ignore if k-bucket is full.
	*/
	void add_node(const std::string & ID, const std::string & IP,
		const std::string & port);

private:
	boost::thread network_thread;
	const std::string ID;
	network::ndgram ndgram;

	/*
	high_node_cache_mutex:
		Locks access to high_node_cache.
	high_node_cache:
		Contains nodes we've seen recently that should be added to k-buckets as
		soon as possible. When a node connects to us with TCP we put the node in
		this container. Add to back, take from front.
	low_node_cache:
		Contains nodes we haven't seen recently that we should try to add to a
		k-bucket only when there are no high_node_cache is empty. Add to back,
		take from front.
	*/
	boost::mutex high_node_cache_mutex;
	std::deque<database::table::peer::info> high_node_cache;
	std::deque<database::table::peer::info> low_node_cache;

	class contact
	{
	public:
		contact(
			const std::string & ID_in,
			const std::string & IP_in,
			const std::string & port_in
		);
		contact(const contact & C);

		const std::string ID;
		const std::string IP;
		const std::string port;

		/*
		do_ping:
			Returns true if ping needs to be sent.
		timed_out:
			Returns true if node timed out and needs to be removed.
		touch:
			Updates the time the node was last seen.
		*/
		bool do_ping();
		bool timed_out();
		void touch();

	private:
		std::time_t last_seen;
		bool ping_sent;
	};

	//k-buckets (see Kademlia paper)
	std::list<contact> Bucket[SHA1::bin_size * 8];

	/*
	calc_bucket:
		Returns bucket number 0-159 that ID belongs in.
	network_loop:
		Loop to send/recv data.
	process_node_cache:
		Called once per second to add nodes to k-buckets.
	*/
	unsigned calc_bucket(const std::string & ID);
	void network_loop();
	void process_node_cache();
};
#endif
