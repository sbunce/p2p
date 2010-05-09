#ifndef H_K_FIND
#define H_K_FIND

//custom
#include "k_find_job.hpp"

class k_find : private boost::noncopyable
{
public:
	/*
	add_local:
		Add a find job, or add endpoints. The map maps distance to endpoint. This
		function adds the result of a local search where we know the distances.
		Optionally a call_back can be specified. If a call_back is not specified
		then a job won't be added if one doesn't already exist.
	find_node:
		Returns std::pair<endpoint, ID_to_find> if find_node message needs to be
		sent.
	recv_host_list:
		Called when host list received.
		Note: ID is the ID returned from find_node().
	remove:
		Remove (if exists) job to find the specified node ID.
	*/
	void add_local(const std::string & ID_to_find,
		const std::multimap<mpa::mpint, net::endpoint> & hosts,
		const boost::function<void (const net::endpoint &, const std::string &)> & call_back
			= boost::function<void (const net::endpoint &, const std::string &)>());
	boost::optional<std::pair<net::endpoint, std::string> > find_node();
	void recv_host_list(const net::endpoint & from, const std::list<net::endpoint> & hosts,
		const std::string & ID_to_find);
	void remove(const std::string & ID_to_find);

private:
	//jobs to find various node IDs
	std::list<boost::shared_ptr<k_find_job> > Job;
};
#endif
