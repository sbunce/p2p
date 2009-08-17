/*
This is the abstract base class for all network states.
*/
#ifndef H_NET_STATE
#define H_NET_STATE

//custom
#include "net_state_persistent.hpp"

//include
#include <boost/enable_shared_from_this.hpp>
#include <network/network.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

class net_state : private boost::noncopyable, public boost::enable_shared_from_this<net_state>
{
public:
	net_state();

	/*
	The network state is entered with this function. The returned state is what
	state to transition to. This can be the current state or a new state.
	*/
	virtual boost::shared_ptr<net_state> enter(network::sock & S) = 0;
};
#endif
