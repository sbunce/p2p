#ifndef H_K_FIND
#define H_K_FIND

//custom
#include "k_find_job.hpp"

class k_find : private boost::noncopyable
{
public:
	/* Add/Cancel Jobs
	find_node:
		Find a specific node. Found endpoints are returned with call_back.
		Note: The call_back may be used multiple times if multiple hosts claim to
			have the specified node ID.
	find_node_cancel:
		Stop looking for specified node ID.
	find_set:
		Find closest protocol_udp::max_store nodest to ID. Found endpoints are
		returned with call_back.
	*/
	void find_node(const std::string & ID_to_find,
		const std::multimap<mpa::mpint, net::endpoint> & hosts,
		const boost::function<void (const net::endpoint &)> & call_back);
	void find_node_cancel(const std::string & ID_to_find);
	void find_set(const std::string & ID,
		const std::multimap<mpa::mpint, net::endpoint> & hosts,
		const boost::function<void (const net::endpoint &)> & call_back);

	/*
	add_to_all:
		Add host for consideration to all find jobs. This is called when a new
		contact is added to a k_bucket.
	recv_host_list:
		Called when host list received.
		Note: ID is the ID returned from find_node().
	send_find_node:
		Returns std::pair<endpoint, ID_to_find> if find_node message needs to be
		sent.
	*/
	void add_to_all(const net::endpoint & ep, const std::string & remote_ID);
	void recv_host_list(const net::endpoint & from, const std::string & remote_ID,
		const std::list<net::endpoint> & hosts, const std::string & ID_to_find);
	std::list<std::pair<net::endpoint, std::string> > send_find_node();

	/*
	tick:
		Called once per second to process timeouts.
	*/
	void tick();

private:
	//job to find one node
	class find_node_element
	{
	public:
		find_node_element(
			const boost::function<void (const net::endpoint &)> & call_back_in,
			boost::shared_ptr<k_find_job> find_job_in
		);
		find_node_element(const find_node_element & FNE);

		//called when a host claims to be the node we're looking for
		const boost::function<void (const net::endpoint &)> call_back;

		/*
		One thing to note is that the same k_find_job may exist in both a
		find_node_element and a find_set_element.
		*/
		boost::shared_ptr<k_find_job> find_job;
	};
	std::map<std::string, find_node_element> Find_Node;

	//job to find a set of nodes
	class find_set_element
	{
	public:
		find_set_element(
			const boost::function<void (const net::endpoint &)> & call_back_in,
			boost::shared_ptr<k_find_job> find_job_in
		);
		find_set_element(const find_set_element & FSE);

		//called when a host claims to be the node we're looking for
		const boost::function<void (const net::endpoint &)> call_back;
		boost::shared_ptr<k_find_job> find_job;

		//returned true if timed out
		bool timeout();
	private:
		//time at which find_set_element was instantiated
		std::time_t time;
	};
	std::map<std::string, find_set_element> Find_Set;
};
#endif
