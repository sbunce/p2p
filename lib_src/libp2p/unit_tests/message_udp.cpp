//custom
#include "../message_udp.hpp"

//include
#include <logger.hpp>
#include <random.hpp>

int fail(0);
const net::buffer test_random(random::urandom(4));
const std::string test_ID("0123456789012345678901234567890123456789");
boost::shared_ptr<net::endpoint> endpoint;

void ping_call_back(const net::endpoint & from,
	const net::buffer & random, const std::string & remote_ID)
{
	if(from != *endpoint){
		LOG; ++fail;
	}
	if(random != test_random){
		LOG; ++fail;
	}
	if(remote_ID != test_ID){
		LOG; ++fail;
	}
}

void pong_call_back(const net::endpoint & from,
	const net::buffer & random, const std::string & remote_ID)
{
	if(from != *endpoint){
		LOG; ++fail;
	}
	if(random != test_random){
		LOG; ++fail;
	}
	if(remote_ID != test_ID){
		LOG; ++fail;
	}
}

void find_node_call_back(const net::endpoint & from,
	const net::buffer & random, const std::string & remote_ID,
	const std::string & ID_to_find)
{
	if(from != *endpoint){
		LOG; ++fail;
	}
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

std::list<net::endpoint> test_hosts;
void host_list_call_back(const net::endpoint & from,
	const std::string & remote_ID, const std::list<net::endpoint> & hosts)
{
	if(from != *endpoint){
		LOG; ++fail;
	}
	if(remote_ID != test_ID){
		LOG; ++fail;
	}
	if(hosts != test_hosts){
		LOG; ++fail;
	}
}

void store_node_call_back(const net::endpoint & from,
	const net::buffer & random, const std::string & remote_ID)
{
	if(from != *endpoint){
		LOG; ++fail;
	}
	if(random != test_random){
		LOG; ++fail;
	}
	if(remote_ID != test_ID){
		LOG; ++fail;
	}
}

const std::string test_hash("1111111111111111111111111111111111111111");
void store_file_call_back(const net::endpoint & from, const net::buffer & random,
	const std::string & remote_ID, const std::string & hash)
{
	if(from != *endpoint){
		LOG; ++fail;
	}
	if(random != test_random){
		LOG; ++fail;
	}
	if(remote_ID != test_ID){
		LOG; ++fail;
	}
	if(hash != test_hash){
		LOG; ++fail;
	}
}

int main()
{
	std::set<net::endpoint> E = net::get_endpoint("localhost", "0");
	assert(!E.empty());

	endpoint.reset(new net::endpoint(*E.begin()));
	boost::shared_ptr<message_udp::recv::base> M_recv;
	boost::shared_ptr<message_udp::send::base> M_send;

	//ping
	M_recv.reset(new message_udp::recv::ping(&ping_call_back));
	M_send.reset(new message_udp::send::ping(test_random, test_ID));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOG; ++fail;
	}

	//pong
	M_recv.reset(new message_udp::recv::pong(&pong_call_back, test_random));
	M_send.reset(new message_udp::send::pong(test_random, test_ID));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOG; ++fail;
	}

	//find_node
	M_recv.reset(new message_udp::recv::find_node(&find_node_call_back));
	M_send.reset(new message_udp::send::find_node(test_random, test_ID, test_ID));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOG; ++fail;
	}

	//host_list
	E = net::get_endpoint("127.0.0.1", "0");
	assert(!E.empty());
	test_hosts.push_back(*E.begin());
	E = net::get_endpoint("127.0.0.2", "0");
	assert(!E.empty());
	//host might not support IPv6, only test IPv4 in this case
	test_hosts.push_back(*E.begin());
	E = net::get_endpoint("::1", "0");
	if(!E.empty()){
		test_hosts.push_back(*E.begin());
	}
	E = net::get_endpoint("::2", "0");
	if(!E.empty()){
		test_hosts.push_back(*E.begin());
	}
	M_recv.reset(new message_udp::recv::host_list(&host_list_call_back, test_random));
	M_send.reset(new message_udp::send::host_list(test_random, test_ID, test_hosts));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOG; ++fail;
	}

	//store_node
	M_recv.reset(new message_udp::recv::store_node(&store_node_call_back));
	M_send.reset(new message_udp::send::store_node(test_random, test_ID));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOG; ++fail;
	}

	//store_file
	M_recv.reset(new message_udp::recv::store_file(&store_file_call_back));
	M_send.reset(new message_udp::send::store_file(test_random, test_ID, test_hash));
	if(!M_recv->recv(M_send->buf, *endpoint)){
		LOG; ++fail;
	}

	return fail;
}
