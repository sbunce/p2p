#ifndef H_K_BUCKET
#define H_K_BUCKET

//custom
#include "k_func.hpp"
#include "protocol_udp.hpp"

//include
#include <atomic_int.hpp>
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
	k_bucket(atomic_int<unsigned> & active_cnt_in);

	/*
	add_reserve:
		Add node to be considered for routing table inclusion. If the node is
		already in the routing table the last_seen time will be updated.
	exists_active:
		Returns true if endpoint exists and is active.
	find_node (two parameters):
		Calculates distance on all nodes in k_bucket and adds them to the hosts
		container.
	find_node (three parameters):
		Same as find_node (two parameters) but excludes 'from' endpoint from
		results.
	ping:
		Returns a endpoint which needs to be pinged.
	recv_pong:
		Called when pong received.
	*/
	void add_reserve(const net::endpoint & ep, const std::string remote_ID);
	bool exists_active(const net::endpoint & ep);
	void find_node(const std::string & ID_to_find,
		std::multimap<mpa::mpint, net::endpoint> & hosts);
	void find_node(const net::endpoint & from, const std::string & ID_to_find,
		std::multimap<mpa::mpint, net::endpoint> & hosts);
	boost::optional<net::endpoint> ping();
	void recv_pong(const net::endpoint & from, const std::string & remote_ID);

private:
	atomic_int<unsigned> & active_cnt;

	class contact : private boost::noncopyable
	{
	public:
		//contact is timed out by default
		contact(
			const net::endpoint & endpoint_in,
			const std::string & remote_ID_in
		);

		const net::endpoint endpoint;
		const std::string remote_ID;

		/*
		active_ping:
			Used on contacts in Bucket_Active. Returns true if ping needs to be
			sent to keep contact from timing out.
			Postcondition: Contact will time out in protocol_udp::response_timeout
				seconds.
		reserve_ping:
			Used on contacts in Bucket_Reserve. Returns true if ping needs to be
			sent. This is used when we want to try to move a contact from reserve
			to active.
			Postcondition: Contact will time out in protocol_udp::response_timeout
				seconds.
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
	std::list<boost::shared_ptr<contact> > Bucket_Active;
	std::list<boost::shared_ptr<contact> > Bucket_Reserve;
};
#endif
