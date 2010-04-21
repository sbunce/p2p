#include "kademlia.hpp"

kademlia::kademlia():
	local_ID(db::table::prefs::get_ID())
{
	//messages to expect anytime
	Exchange.expect_anytime(boost::shared_ptr<message_udp::recv::base>(
		new message_udp::recv::ping(boost::bind(&kademlia::recv_ping, this, _1, _2, _3))));
	Exchange.expect_anytime(boost::shared_ptr<message_udp::recv::base>(
		new message_udp::recv::find_node(boost::bind(&kademlia::recv_find_node,
		this, _1, _2, _3, _4))));

	//start internal thread
	main_loop_thread = boost::thread(boost::bind(&kademlia::main_loop, this));
}

kademlia::~kademlia()
{
	main_loop_thread.interrupt();
	main_loop_thread.join();
}

void kademlia::ping()
{
	std::set<network::endpoint> hosts = Route_Table.ping();
	for(std::set<network::endpoint>::iterator it_cur = hosts.begin(),
		it_end = hosts.end(); it_cur != it_end; ++it_cur)
	{
		LOG << it_cur->IP() << " " << it_cur->port();
		network::buffer random(portable_urandom(4));
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::ping(random, local_ID)), *it_cur);
		Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
			new message_udp::recv::pong(boost::bind(&kademlia::recv_pong, this, _1, _2), random)),
			*it_cur);
	}
}

void kademlia::main_loop()
{
	//restore hosts from database, randomize and add to route_table
	std::vector<db::table::peer::info> hosts = db::table::peer::resume();
	std::random_shuffle(hosts.begin(), hosts.end());
	while(!hosts.empty()){
		std::set<network::endpoint> E = network::get_endpoint(hosts.back().IP,
			hosts.back().port, network::udp);
		if(E.empty()){
			LOG << "failed \"" << hosts.back().IP << "\" " << hosts.back().port;
		}else{
			Route_Table.add_reserve(hosts.back().ID, *E.begin());
		}
		hosts.pop_back();
	}

	//main loop
	std::time_t last_time(std::time(NULL));
	while(true){
		boost::this_thread::interruption_point();
		if(last_time != std::time(NULL)){
			//once per second
			ping();
			last_time = std::time(NULL);
		}
		Exchange.tick();
	}
}

void kademlia::recv_find_node(const network::endpoint & endpoint,
	const network::buffer & random, const std::string & remote_ID,
	const std::string & ID_to_find)
{
	LOG << endpoint.IP() << " " << endpoint.port() << " remote_ID: " << remote_ID
		<< " find: " << ID_to_find;
	Route_Table.add_reserve(remote_ID, endpoint);
	std::list<std::pair<network::endpoint, unsigned char> >
		hosts = Route_Table.find_node(remote_ID, ID_to_find);
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::host_list(random, local_ID, hosts)), endpoint);
}

void kademlia::recv_ping(const network::endpoint & endpoint, const network::buffer & random,
	const std::string & remote_ID)
{
	LOG << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	Route_Table.add_reserve(remote_ID, endpoint);
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::pong(random, local_ID)), endpoint);
}

void kademlia::recv_pong(const network::endpoint & endpoint, const std::string & remote_ID)
{
	LOG << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	Route_Table.pong(remote_ID, endpoint);
}
