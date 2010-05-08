#ifndef H_K_FIND
#define H_K_FIND

//custom
#include "k_find_job.hpp"

class k_find : private boost::noncopyable
{
public:
	/*

	*/
	void add(const std::string & ID, const std::list<net::endpoint> & hosts);

	/*
	timeout_call_back:
		Called when find_node request times out.
	*/
	void timeout_call_back(const std::string ID_to_find,
		const net::endpoint endpoint);

private:
	std::list<boost::shared_ptr<k_find_job> > Job;
};
#endif
