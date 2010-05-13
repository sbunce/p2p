#ifndef H_K_FIND
#define H_K_FIND

//custom
#include "k_find_job.hpp"

class k_find : private boost::noncopyable
{
public:
	/* Add or Cancel Jobs
	add_job:
		Add a find job. If a find job already exists this function does nothing.
		The call_back is called when a host claiming to have the specified ID is
		found.
		Note: The call_back may be used multiple times if multiple hosts claim to
			have the specified node ID.
	cancel_job:
		Stop looking for the specified node ID.
	*/
	void add_job(const std::string & ID_to_find,
		const std::multimap<mpa::mpint, net::endpoint> & hosts,
		const boost::function<void (const net::endpoint &, const std::string)> & call_back);
	void cancel_job(const std::string & ID_to_find);

	/*
	add_to_all:
		Add host for consideration to all find jobs. This is called when a new
		contact is added to a k_bucket.
	find_node:
		Returns std::pair<endpoint, ID_to_find> if find_node message needs to be
		sent.
	recv_host_list:
		Called when host list received.
		Note: ID is the ID returned from find_node().
	*/
	void add_to_all(const net::endpoint & ep, const std::string & remote_ID);
	std::list<std::pair<net::endpoint, std::string> > find_node();
	void recv_host_list(const net::endpoint & from, const std::string & remote_ID,
		const std::list<net::endpoint> & hosts, const std::string & ID_to_find);

private:
	//maps ID_to_find to find job
	std::map<std::string, boost::shared_ptr<k_find_job> > Jobs;

	//last ID serviced, used to rotate through jobs
	std::string last_job;
};
#endif
