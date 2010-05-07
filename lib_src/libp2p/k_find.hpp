#ifndef H_K_FIND
#define H_K_FIND

//include
#include <boost/utility.hpp>
#include <net/net.hpp>

class k_find : private boost::noncopyable
{
public:
	k_find(
		const std::string & ID_to_find_in,
		const boost::function<void (const net::endpoint)> & call_back_in
	);

	const std::string ID_to_find;
	const boost::function<void (const net::endpoint)> call_back;

	/*
	add_endpoint:
		Add 
	find_node:
		Returns endpoint to send find_node message to.
	*/
	//boost::optional<network::endpoint> find_node();
	//void recv_host_list(const network::endpoint & endpoint,
		//const std::list<std::pair<net::endpoint, unsigned char> > & hosts);
};
#endif
