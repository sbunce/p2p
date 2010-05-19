#ifndef H_K_FIND_JOB
#define H_K_FIND_JOB

//custom
#include "k_contact.hpp"
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
	k_find_job(const std::multimap<mpa::mpint, net::endpoint> & hosts);

	enum call_back_t{
		exact_match, //call back only done for exact match
		any_will_do  //call backs done for all results
	};

	/*
	add:
		Add endpoint where we know the distance it is from ID to find.
	find_node:
		Returns list of endpoints that find_node needs to be sent to.
	found:
		Returns reference to nodes that have been found.
	recv_host_list:
		Called when host list received. The dist parameter is distance from remote
		host to ID to find.
	register_call_back:
		Register call back. If exact_match = true only call back on exact node.
		Note: This can be called multiple times.
		Note: If exact_match = false this function uses the call back for all
			endpoints in Found immediately.
	timeout:
		Returns true if the k_find_job has expired.
	*/
	void add(const net::endpoint & ep, const mpa::mpint & dist);
	std::list<net::endpoint> find_node();
	const std::multimap<mpa::mpint, net::endpoint> & found();
	void recv_host_list(const net::endpoint & from,
		const std::list<net::endpoint> & hosts, const mpa::mpint & dist);
	void register_call_back(const boost::function<void (const net::endpoint &)> & call_back,
		const call_back_t type);
	bool timeout();

private:
	//time at which instantiated
	std::time_t time;

	class store_element
	{
	public:
		store_element(
			const net::endpoint & endpoint_in,
			const k_contact & contact_in
		);
		store_element(const store_element & SE);

		const net::endpoint endpoint;
		k_contact contact;
	};

	/*
	Sorts contacts by distance. We add to this when we receive a host_list.
	Note: We don't know the distance of endpoints in a host list so we assume
		they're one less than the host that sent the host_list.
	Note: If a contact times out we set it's distance to maximum, effectively
		deprioritizing it.
	*/
	std::multimap<mpa::mpint, store_element> Store;

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

	class call_back_element
	{
	public:
		call_back_element(
			const boost::function<void (const net::endpoint &)> call_back_in,
			const call_back_t type_in
		);
		call_back_element(const call_back_element & CBE);
		const boost::function<void (const net::endpoint &)> call_back;
		const call_back_t type;
	};

	//registered call backs
	std::list<call_back_element> Call_Back;

	/*
	call_back:
		Do any call backs which need to be done for endpoint.
	*/
	void call_back(const net::endpoint & ep, const mpa::mpint & dist);
};
#endif
