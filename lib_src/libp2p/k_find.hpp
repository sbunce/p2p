#ifndef H_K_FIND
#define H_K_FIND

//custom
#include "k_find_job.hpp"

class k_find : private boost::noncopyable
{
public:
	/* Find
	node:
		Find a specific node. Found endpoints are returned with call_back.
		Note: The call_back may be used multiple times if multiple hosts claim to
			have the specified node ID.
	set:
		Find closest protocol_udp::max_store nodes to ID. Found endpoints are
		returned with call_back.
	*/
	void node(const std::string & ID,
		const std::multimap<mpa::mpint, net::endpoint> & hosts,
		const boost::function<void (const net::endpoint &)> & call_back);
	void set(const std::string & ID,
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
		Returns info for find_node requests that need to be sent.
	*/
	void add_to_all(const net::endpoint & ep, const std::string & remote_ID);
	void recv_host_list(const net::endpoint & from, const std::string & remote_ID,
		const std::list<net::endpoint> & hosts, const std::string & ID_to_find);
	std::list<std::pair<net::endpoint, std::string> > send_find_node();

	/* Timed Functions
	tick:
		Called once per second to process timeouts.
	*/
	void tick();

private:
	std::map<std::string, boost::shared_ptr<k_find_job> > Find;
};
#endif
