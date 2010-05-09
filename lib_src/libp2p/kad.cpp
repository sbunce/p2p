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

void kad::find_node_add(const std::string & ID_to_find,
	const boost::function<void (const net::endpoint &, const std::string &)> & call_back)
{
	boost::mutex::scoped_lock lock(relay_job_mutex);
	relay_job.push_back(boost::bind(&kad::find_node_add_relay, this, ID_to_find, call_back));
}

void kad::find_node_remove(const std::string & ID)
{
	boost::mutex::scoped_lock lock(relay_job_mutex);
	relay_job.push_back(boost::bind(&kad::find_node_remove_relay, this, ID));
}

void kad::find_node_add_relay(const std::string ID_to_find,
	const boost::function<void (const net::endpoint &, const std::string &)> call_back)
{
	std::multimap<mpa::mpint, net::endpoint> hosts = Route_Table.find_node_local(ID_to_find);
	Find.add_local(ID_to_find, hosts, call_back);
}

void kad::find_node_remove_relay(const std::string ID)
{
	Find.remove(ID);
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
			Route_Table.add_reserve(*E.begin(), hosts.back().ID);
		}
		hosts.pop_back();
	}

	//main loop
	std::time_t last_time(std::time(NULL));
	while(true){
		boost::this_thread::interruption_point();
		if(last_time != std::time(NULL)){
			//once per second
			send_find_node();
			send_ping();
			last_time = std::time(NULL);
		}
		Exchange.tick();
		process_relay_job();
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

void kad::recv_find_node(const net::endpoint & from,
	const net::buffer & random, const std::string & remote_ID,
	const std::string & ID_to_find)
{
	LOG << from.IP() << " " << from.port() << " remote_ID: " << remote_ID
		<< " find: " << ID_to_find;
	Route_Table.add_reserve(from, remote_ID);
	std::list<net::endpoint> hosts = Route_Table.find_node(ID_to_find);
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::host_list(random, local_ID, hosts)), from);
}

void kad::recv_host_list(const net::endpoint & from,
	const std::string & remote_ID, const std::list<net::endpoint> & hosts,
	const std::string ID_to_find)
{
	LOG << from.IP() << " " << from.port() << " " << remote_ID;
	Find.recv_host_list(from, hosts, ID_to_find);
}

void kad::recv_ping(const net::endpoint & from,
	const net::buffer & random, const std::string & remote_ID)
{
	LOG << from.IP() << " " << from.port() << " " << remote_ID;
	Route_Table.add_reserve(from, remote_ID);
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::pong(random, local_ID)), from);
}

void kad::recv_pong(const net::endpoint & from, const std::string & remote_ID)
{
	LOG << from.IP() << " " << from.port() << " " << remote_ID;
	Route_Table.recv_pong(from, remote_ID);
}

void kad::send_find_node()
{
	boost::optional<std::pair<net::endpoint, std::string> > info = Find.find_node();
	if(info){
		LOG << info->first.IP() << " " << info->first.port();
		net::buffer random(portable_urandom(4));
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::find_node(random, local_ID, info->second)), info->first);
		Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
			new message_udp::recv::host_list(boost::bind(&kad::recv_host_list,
			this, _1, _2, _3, info->second), random)), info->first);
	}
}

void kad::send_ping()
{
	boost::optional<net::endpoint> ep = Route_Table.ping();
	if(ep){
		LOG << ep->IP() << " " << ep->port();
		net::buffer random(portable_urandom(4));
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::ping(random, local_ID)), *ep);
		Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
			new message_udp::recv::pong(boost::bind(&kad::recv_pong, this, _1, _2), random)),
			*ep);
	}
}
