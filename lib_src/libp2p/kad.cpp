#include "kad.hpp"

kad::kad():
	local_ID(db::table::prefs::get_ID())
{
	//messages to expect anytime
	Exchange.expect_anytime(boost::shared_ptr<message_udp::recv::base>(
		new message_udp::recv::ping(boost::bind(&kad::recv_ping, this, _1, _2, _3))));
	Exchange.expect_anytime(boost::shared_ptr<message_udp::recv::base>(
		new message_udp::recv::find_node(boost::bind(&kad::recv_find_node,
		this, _1, _2, _3, _4))));

	//start networking
	network_thread = boost::thread(boost::bind(&kad::network_loop, this));
}

kad::~kad()
{
	network_thread.interrupt();
	network_thread.join();
}

void kad::network_loop()
{
	//restore hosts from database, randomize to try different nodes each time
	std::vector<db::table::peer::info> hosts = db::table::peer::resume();
	std::random_shuffle(hosts.begin(), hosts.end());
	while(!hosts.empty()){
		std::set<net::endpoint> E = net::get_endpoint(hosts.back().IP,
			hosts.back().port);
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
			send_ping();
			last_time = std::time(NULL);
		}
		Exchange.tick();
	}
}

void kad::process_relay_job()
{
	while(true){
		boost::function<void ()> tmp;
		{//begin lock scope
		boost::mutex::scoped_lock lock(relay_job_mutex);
		if(relay_job.empty()){
			break;
		}
		tmp = relay_job.front();
		relay_job.pop_front();
		}//end lock scope
		tmp();
	}
}

void kad::recv_find_node(const net::endpoint & endpoint,
	const net::buffer & random, const std::string & remote_ID,
	const std::string & ID_to_find)
{
	LOG << endpoint.IP() << " " << endpoint.port() << " remote_ID: " << remote_ID
		<< " find: " << ID_to_find;
	Route_Table.add_reserve(remote_ID, endpoint);
	std::list<std::pair<net::endpoint, unsigned char> >
		hosts = Route_Table.find_node(remote_ID, ID_to_find);
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::host_list(random, local_ID, hosts)), endpoint);
}

void kad::recv_ping(const net::endpoint & endpoint, const net::buffer & random,
	const std::string & remote_ID)
{
	LOG << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	Route_Table.add_reserve(remote_ID, endpoint);
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::pong(random, local_ID)), endpoint);
}

void kad::recv_pong(const net::endpoint & endpoint, const std::string & remote_ID)
{
	LOG << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	Route_Table.pong(remote_ID, endpoint);
}

void kad::send_ping()
{
	boost::optional<net::endpoint> endpoint = Route_Table.ping();
	if(endpoint){
		LOG << endpoint->IP() << " " << endpoint->port();
		net::buffer random(portable_urandom(4));
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::ping(random, local_ID)), *endpoint);
		Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
			new message_udp::recv::pong(boost::bind(&kad::recv_pong, this, _1, _2), random)),
			*endpoint);
	}
}
