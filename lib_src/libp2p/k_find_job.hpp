#ifndef H_K_FIND_JOB
#define H_K_FIND_JOB

//custom
#include "protocol_udp.hpp"

//include
#include <boost/utility.hpp>
#include <mpa.hpp>
#include <net/net.hpp>

class k_find_job : private boost::noncopyable
{
public:
	k_find_job(
		const std::string & ID_to_find_in,
		const boost::function<void (const net::endpoint &, const std::string &)> & call_back_in
	);

	//ID this job is looking for
	const std::string ID_to_find;

	/*
	Called when we find a node that claims to be the node we're looking for.
	Note: The node might be lying.
	*/
	const boost::function<void (const net::endpoint &, const std::string &)> call_back;

	/*
	add_local:
		Add the results of a local find_node search where we know distances.
	find_node:
		Returns endpoint to send find_node message to.
	recv_host_list:
		Called when host list received.
	*/
	void add_local(const std::multimap<mpa::mpint, net::endpoint> & hosts);
	boost::optional<net::endpoint> find_node();
	void recv_host_list(const net::endpoint & from, const std::list<net::endpoint> & hosts);

private:
	class contact : private boost::noncopyable
	{
	public:
		explicit contact(const net::endpoint & endpoint_in);

		const net::endpoint endpoint;

		/*
		send:
			Returns true if find_node should be sent to endpoint.
		*/
		bool send();

	private:
		std::time_t time_sent;  //when find_node was last sent
		bool find_node_sent;    //true if find_node sent and waiting for response
		unsigned find_node_cnt; //number of find_node messages sent
	};

	/*
	This container holds hosts to send find_node requests to. It is sorted by
	distance. We add to this container when we receive a host_list. We take the
	node ID of the host that sent the host_list and assume the distances of the
	hosts in the list are less. The furthest node in the host_list will be one
	less, the second furthest will be two less etc.
	*/
	std::multimap<mpa::mpint, boost::shared_ptr<contact> > Store;
};
#endif
