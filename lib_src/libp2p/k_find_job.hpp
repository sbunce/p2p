#ifndef H_K_FIND_JOB
#define H_K_FIND_JOB

//custom
#include "k_func.hpp"
#include "protocol_udp.hpp"

//include
#include <boost/utility.hpp>
#include <mpa.hpp>
#include <net/net.hpp>

//standard
#include <algorithm>

class k_find_job : private boost::noncopyable
{
public:
	k_find_job(const boost::function<void (const net::endpoint &)> & call_back_in);

	/*
	When a host is found with distance zero this call back is used.
	Note: This may be called multiple times if multiple hosts claim to be the
		ID we're looking for.
	Note: The ID is bound to this function object in k_find.
	*/
	const boost::function<void (const net::endpoint &)> call_back;

	/*
	add:
		Add endpoint where we know the distance it is from ID to find.
	add_initial:
		Adds endpoints but staggers the delays. Used to add the results of a local
		find node search after find job is created.
		Precondition: No endpoints must have been added to the k_find_node.
	find_node:
		Returns list of endpoints that find_node needs to be sent to.
	recv_host_list:
		Called when host list received. The dist parameter is distance from remote
		host to ID to find.
	*/
	void add(const mpa::mpint & dist, const net::endpoint & ep);
	void add_initial(const std::multimap<mpa::mpint, net::endpoint> & hosts);
	std::list<net::endpoint> find_node();
	void recv_host_list(const net::endpoint & from,
		const std::list<net::endpoint> & hosts, const mpa::mpint & dist);

private:

/* IDEA
Have delay parameter to contact. The delay is a delay (in second) until find_node
request will be sent.

Have a add_initial which adds contacts and sets the timeouts in 1 second
intervals. With the lowest delay going to the lowest distance and the highest
delay going to the highest distance.

The first few contacts can be set with no delay.
*/

	class contact : private boost::noncopyable
	{
	public:
		explicit contact(
			const net::endpoint & endpoint_in,
			const unsigned delay = 0
		);
		const net::endpoint endpoint;

		/*
		send:
			Returns true if find_node should be sent to endpoint.
		timeout:
			Returns true if contact has timed out.
			Postcondition: timeout() will return false. The contact is reset to
				original state with no delay.
		timeout_cnt:
			Returns number of times the contact has timed out.
		*/
		bool send();
		bool timeout();
		unsigned timeout_count();

	private:
		/*
		time:
			Initially set to the time the contact was instantiated. After send()
			returns true this is used to check for timeout.
		delay:
			The number of seconds to delay send() after instantiation.
		sent:
			Set to true when send() has returned true. Enables timeout of the
			contact.
		timeout_cnt:
			Returns the number of times this contact has timed out. Used for
			retransmission limit.
		*/
		std::time_t time;
		unsigned delay;
		bool sent;
		unsigned timeout_cnt;
	};

	/*
	Sorts contacts by distance. We add to this when we receive a host_list.
	Note: We don't know the distance of endpoints in a host list so we assume
		they're one less than the host that sent the host_list.
	Note: If a contact times out we set it's distance to maximum, effectively
		deprioritizing it.
	*/
	std::multimap<mpa::mpint, boost::shared_ptr<contact> > Store;

	/*
	After we receive a host_list we add the endpoint which sent it to this
	container. This container is kept sized to max number of endpoints we will
	send store messages to.
	Note: The distance of the endpoint is recalculated because it may not be
		accurate (see Store documentation).
	*/
	std::multimap<mpa::mpint, net::endpoint> Found;

	//memoize endpoints to eliminate duplicate find_node requests
	std::set<net::endpoint> Memoize;
};
#endif
