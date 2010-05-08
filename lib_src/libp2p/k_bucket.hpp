#ifndef H_K_BUCKET
#define H_K_BUCKET

//custom
#include "k_func.hpp"
#include "protocol_udp.hpp"

//include
#include <boost/optional.hpp>
#include <mpa.hpp>
#include <net/net.hpp>

//standard
#include <ctime>
#include <list>
#include <map>
#include <string>

class k_bucket : private boost::noncopyable
{
public:
	/*
	add_reserve:
		Add node to be considered for routing table inclusion. If the node is
		already in the routing table the last_seen time will be updated.
	find_node:
		Calculates distance on all nodes in k_bucket and adds them to the hosts
		container. Highest distance elements are removed from the hosts container
		such that it is no larger than protocol_udp::k_bucket_size.
		std::map<distance, std::pair<remote_ID, endpoint> >
	ping:
		Returns a endpoint which needs to be pinged.
	pong:
		Attempts to send ping. Returns true if ping sent.
	*/
	void add_reserve(const net::endpoint & endpoint, const std::string remote_ID);
	void find_node(const std::string & ID_to_find, const mpa::mpint & max_dist,
		std::multimap<mpa::mpint, net::endpoint> & hosts);
	boost::optional<net::endpoint> ping();

	/* Call Backs
	recv_pong:
		Called when pong received.
	*/
	void recv_pong(const net::endpoint & from, const std::string & remote_ID);

private:
	class contact
	{
	public:
		//contact is timed out by default
		contact(
			const net::endpoint & endpoint_in,
			const std::string & remote_ID_in
		);
		contact(const contact & C);

		const net::endpoint endpoint;
		const std::string remote_ID;

		/*
		active_ping:
			Used on contacts in Bucket_Active. Returns true if ping needs to be
			sent to keep contact from timing out. Returns true 60 seconds before
			1 hour timeout.
		reserve_ping:
			Used on contacts in Bucket_Reserve. Returns true if ping can be sent.
			This is used when we want to try to move a contact from reserve to
			active.
			Postcondition: Enables timeout for 60s.
		timed_out:
			Returns true if contact has timed out and needs to be removed.
		touch:
			Updates last seen time of element.
			Postcondition: Sets the contact to timeout to 1 hour.
		*/
		bool active_ping();
		bool reserve_ping();
		bool timed_out();
		void touch();

	private:
		std::time_t last_seen;
		bool ping_sent;
	};

	/*
	Contacts in Bucket_Active are pinged to make sure they're still up. When a
	active contact times out a replacement will be gotten from reserve.
	*/
	std::list<contact> Bucket_Active;
	std::list<contact> Bucket_Reserve;
};
#endif
