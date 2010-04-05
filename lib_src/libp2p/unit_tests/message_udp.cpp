//custom
#include "../message_udp.hpp"

//include
#include <logger.hpp>
#include <random.hpp>

int fail(0);
const network::buffer test_random(portable_urandom(8));
const std::string test_ID("0123456789012345678901234567890123456789");
boost::shared_ptr<network::endpoint> endpoint;

void ping_call_back(const network::endpoint & endpoint,
	const network::buffer & random)
{
	if(random != test_random){
		LOGGER; ++fail;
	}
}

void pong_call_back(const network::endpoint & endpoint,
	const std::string & remote_ID)
{
	if(remote_ID != test_ID){
		LOGGER; ++fail;
	}
}

void find_node_call_back(const network::endpoint & endpoint,
	const network::buffer & random, const std::string & ID_to_find)
{
	if(random != test_random){
		LOGGER; ++fail;
	}
	if(ID_to_find != test_ID){
		LOGGER; ++fail;
	}
}

int main()
{
	network::start();
	std::set<network::endpoint> endpoint_set = network::get_endpoint("localhost", "0", network::udp);
	assert(!endpoint_set.empty());
	endpoint = boost::shared_ptr<network::endpoint>(new network::endpoint(*endpoint_set.begin()));
	boost::shared_ptr<message_udp::recv::base> M_recv;
	boost::shared_ptr<message_udp::send::base> M_send;

	//ping
	M_recv = boost::shared_ptr<message_udp::recv::base>(new message_udp::recv::ping(
		&ping_call_back));
	M_send = boost::shared_ptr<message_udp::send::base>(new message_udp::send::ping(
		test_random));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOGGER; ++fail;
	}

	//pong
	M_recv = boost::shared_ptr<message_udp::recv::base>(new message_udp::recv::pong(
		&pong_call_back, test_random));
	M_send = boost::shared_ptr<message_udp::send::base>(new message_udp::send::pong(
		test_random, test_ID));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOGGER; ++fail;
	}

	//find_node
	M_recv = boost::shared_ptr<message_udp::recv::base>(new message_udp::recv::find_node(
		&find_node_call_back));
	M_send = boost::shared_ptr<message_udp::send::base>(new message_udp::send::find_node(
		test_random, test_ID));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOGGER; ++fail;
	}

	return fail;
}
