//custom
#include "../message_udp.hpp"

//include
#include <logger.hpp>
#include <random.hpp>

int fail(0);
const network::buffer test_random(portable_urandom(4));
const std::string test_ID("0123456789012345678901234567890123456789");
boost::shared_ptr<network::endpoint> endpoint;

void ping_call_back(const network::endpoint & endpoint,
	const network::buffer & random, const std::string & remote_ID)
{
	if(random != test_random){
		LOG; ++fail;
	}
	if(remote_ID != test_ID){
		LOG; ++fail;
	}
}

void pong_call_back(const network::endpoint & endpoint,
	const std::string & remote_ID)
{
	if(remote_ID != test_ID){
		LOG; ++fail;
	}
}

void find_node_call_back(const network::endpoint & endpoint,
	const network::buffer & random, const std::string & remote_ID,
	const std::string & ID_to_find)
{
	if(random != test_random){
		LOG; ++fail;
	}
	if(remote_ID != test_ID){
		LOG; ++fail;
	}
	if(ID_to_find != test_ID){
		LOG; ++fail;
	}
}

std::list<std::pair<network::endpoint, unsigned char> > test_hosts;
void host_list_call_back(const network::endpoint & endpoint,
	const std::string & remote_ID,
	const std::list<std::pair<network::endpoint, unsigned char> > & hosts)
{
	if(remote_ID != test_ID){
		LOG; ++fail;
	}
	if(hosts != test_hosts){
		LOG; ++fail;
	}
}

int main()
{
	std::set<network::endpoint> E = network::get_endpoint("localhost", "0");
	assert(!E.empty());
	endpoint = boost::shared_ptr<network::endpoint>(new network::endpoint(*E.begin()));
	boost::shared_ptr<message_udp::recv::base> M_recv;
	boost::shared_ptr<message_udp::send::base> M_send;

	//ping
	M_recv = boost::shared_ptr<message_udp::recv::base>(new message_udp::recv::ping(
		&ping_call_back));
	M_send = boost::shared_ptr<message_udp::send::base>(new message_udp::send::ping(
		test_random, test_ID));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOG; ++fail;
	}

	//pong
	M_recv = boost::shared_ptr<message_udp::recv::base>(new message_udp::recv::pong(
		&pong_call_back, test_random));
	M_send = boost::shared_ptr<message_udp::send::base>(new message_udp::send::pong(
		test_random, test_ID));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOG; ++fail;
	}

	//find_node
	M_recv = boost::shared_ptr<message_udp::recv::base>(new message_udp::recv::find_node(
		&find_node_call_back));
	M_send = boost::shared_ptr<message_udp::send::base>(new message_udp::send::find_node(
		test_random, test_ID, test_ID));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOG; ++fail;
	}

	//host_list
	/*
	Note: host_list can be reordered when sent. For this test we intentionally
		put all IPv4 addresses first in the list so they don't get reordered.
	*/
	E = network::get_endpoint("127.0.0.1", "0");
	assert(!E.empty());
	test_hosts.push_back(std::make_pair(*E.begin(), 0));
	E = network::get_endpoint("127.0.0.2", "0");
	assert(!E.empty());
	test_hosts.push_back(std::make_pair(*E.begin(), 0));
	E = network::get_endpoint("::1", "0");
	if(!E.empty()){
		test_hosts.push_back(std::make_pair(*E.begin(), 0));
	}
	E = network::get_endpoint("::2", "0");
	if(!E.empty()){
		test_hosts.push_back(std::make_pair(*E.begin(), 0));
	}
	M_recv = boost::shared_ptr<message_udp::recv::base>(new message_udp::recv::host_list(
		&host_list_call_back, test_random));
	M_send = boost::shared_ptr<message_udp::send::base>(new message_udp::send::host_list(
		test_random, test_ID, test_hosts));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOG; ++fail;
	}

	return fail;
}
