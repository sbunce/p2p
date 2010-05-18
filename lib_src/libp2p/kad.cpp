#include "kad.hpp"

//BEGIN store_token
kad::store_token::store_token(
	const net::buffer & random_in,
	const direction_t direction
):
	random(random_in),
	time(std::time(NULL)),
	_timeout(direction == incoming ?
		protocol_udp::store_token_incoming_timeout :
		protocol_udp::store_token_outgoing_timeout
	)
{

}

kad::store_token::store_token(const store_token & ST):
	random(ST.random),
	time(ST.time),
	_timeout(ST._timeout)
{

}

bool kad::store_token::timeout()
{
	return std::time(NULL) - time > _timeout;
}
//END store_token

kad::kad():
	local_ID(db::table::prefs::get_ID()),
	active_cnt(0),
	Route_Table(active_cnt, boost::bind(&kad::route_table_call_back, this, _1, _2)),
	send_ping_called(false)
{
	//messages to expect anytime
	Exchange.expect_anytime(boost::shared_ptr<message_udp::recv::base>(
		new message_udp::recv::ping(boost::bind(&kad::recv_ping, this, _1, _2, _3))));
	Exchange.expect_anytime(boost::shared_ptr<message_udp::recv::base>(
		new message_udp::recv::find_node(boost::bind(&kad::recv_find_node,
		this, _1, _2, _3, _4))));
	Exchange.expect_anytime(boost::shared_ptr<message_udp::recv::base>(
		new message_udp::recv::store_node(boost::bind(&kad::recv_store_node,
		this, _1, _2, _3))));

	//start networking
	network_thread = boost::thread(boost::bind(&kad::network_loop, this));
}

kad::~kad()
{
	network_thread.interrupt();
	network_thread.join();
}

unsigned kad::count()
{
	return active_cnt;
}

unsigned kad::download_rate()
{
	return Exchange.download_rate();
}

void kad::find_node(const std::string & ID_to_find,
	const boost::function<void (const net::endpoint &)> & call_back)
{
	boost::mutex::scoped_lock lock(relay_job_mutex);
	relay_job.push_back(boost::bind(&kad::find_node_relay, this, ID_to_find, call_back));
}

void kad::find_node_cancel(const std::string & ID)
{
	boost::mutex::scoped_lock lock(relay_job_mutex);
	relay_job.push_back(boost::bind(&kad::find_node_cancel_relay, this, ID));
}

void kad::find_node_relay(const std::string ID_to_find,
	const boost::function<void (const net::endpoint &)> call_back)
{
	std::multimap<mpa::mpint, net::endpoint> hosts = Route_Table.find_node_local(ID_to_find);
	Find.find_node(ID_to_find, hosts, call_back);
}

void kad::find_node_cancel_relay(const std::string ID)
{
	Find.find_node_cancel(ID);
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
	std::time_t second_timeout(std::time(NULL));
	std::time_t hour_timeout(std::time(NULL));
	while(true){
		boost::this_thread::interruption_point();
		if(std::time(NULL) > second_timeout){
			send_find_node();
			send_ping();
			store_token_timeout();
			Find.tick();
			second_timeout = std::time(NULL) + 1;
		}
		if(std::time(NULL) > hour_timeout){
			send_store_node();
			hour_timeout = std::time(NULL) + 60 * 60;
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
	LOG << from.IP() << " " << from.port() << " find: " << ID_to_find;
	Route_Table.add_reserve(from, remote_ID);
	std::list<net::endpoint> hosts;
	hosts = Route_Table.find_node(from, ID_to_find);
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::host_list(random, local_ID, hosts)), from);
}

void kad::recv_host_list(const net::endpoint & from,
	const std::string & remote_ID, const std::list<net::endpoint> & hosts,
	const std::string ID_to_find)
{
	LOG << from.IP() << " " << from.port() << " " << remote_ID;
	Route_Table.recv_pong(from, remote_ID);
	for(std::list<net::endpoint>::const_iterator it_cur = hosts.begin(),
		it_end = hosts.end(); it_cur != it_end; ++it_cur)
	{
		Route_Table.add_reserve(*it_cur);
	}
	Find.recv_host_list(from, remote_ID, hosts, ID_to_find);
}

void kad::recv_ping(const net::endpoint & from,
	const net::buffer & random, const std::string & remote_ID)
{
	Route_Table.add_reserve(from, remote_ID);
	outgoing_store_token.insert(std::make_pair(from, store_token(random,
		store_token::outgoing)));
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::pong(random, local_ID)), from);
}

void kad::recv_pong(const net::endpoint & from, const net::buffer & random,
	const std::string & remote_ID)
{
	incoming_store_token.insert(std::make_pair(from, store_token(random,
		store_token::incoming)));
	Route_Table.recv_pong(from, remote_ID);
}

void kad::recv_store_node(const net::endpoint & from, const net::buffer & random,
	const std::string & remote_ID)
{
	typedef std::multimap<net::endpoint, store_token>::iterator it_t;
	std::pair<it_t, it_t> p = outgoing_store_token.equal_range(from);
	bool found = false;
	for(; p.first != p.second; ++p.first){
		if(p.first->second.random == random){
			found = true;
			break;
		}
	}
	if(found){
		LOG << from.IP() << " " << from.port() << " " << remote_ID;
		db::table::peer::add(db::table::peer::info(remote_ID, from.IP(), from.port()));
	}else{
		LOG << "invalid: " << from.IP() << " " << from.port() << " " << remote_ID;
	}
}

void kad::route_table_call_back(const net::endpoint & ep, const std::string & remote_ID)
{
	Find.add_to_all(ep, remote_ID);
}

void kad::send_find_node()
{
	std::list<std::pair<net::endpoint, std::string> > jobs = Find.send_find_node();
	for(std::list<std::pair<net::endpoint, std::string> >::iterator
		it_cur = jobs.begin(), it_end = jobs.end(); it_cur != it_end; ++it_cur)
	{
		net::buffer random(random::urandom(4));
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::find_node(random, local_ID, it_cur->second)), it_cur->first);
		Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
			new message_udp::recv::host_list(boost::bind(&kad::recv_host_list,
			this, _1, _2, _3, it_cur->second), random)), it_cur->first);
	}
}

void kad::send_ping()
{
	//ping everything possible on the first call
	boost::optional<net::endpoint> ep;
	do{
		ep = Route_Table.ping();
		if(ep){
			net::buffer random(random::urandom(4));
			Exchange.send(boost::shared_ptr<message_udp::send::base>(
				new message_udp::send::ping(random, local_ID)), *ep);
			Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
				new message_udp::recv::pong(boost::bind(&kad::recv_pong, this, _1, _2, _3), random)),
				*ep);
		}
	}while(!send_ping_called && ep);
	send_ping_called = true;
}

void kad::send_store_node()
{
	LOG << "store_node started";
	std::multimap<mpa::mpint, net::endpoint> hosts = Route_Table.find_node_local(local_ID);
	Find.find_set(local_ID, hosts, boost::bind(&kad::send_store_node_call_back, this, _1));
}

void kad::send_store_node_call_back(const net::endpoint & from,
	const net::buffer & random, const std::string & remote_ID)
{
	recv_pong(from, random, remote_ID);
	LOG << "store_node: " << from.IP() << " " << from.port();
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::store_node(random, local_ID)), from);
}

void kad::send_store_node_call_back(const net::endpoint & ep)
{
	std::multimap<net::endpoint, store_token>::iterator it = incoming_store_token.find(ep);
	if(it == incoming_store_token.end()){
		//do ping to get store token
		LOG << "ping: " << ep.IP() << " " << ep.port();
		net::buffer random(random::urandom(4));
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::ping(random, local_ID)), ep);
		Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
			new message_udp::recv::pong(boost::bind(&kad::send_store_node_call_back,
			this, _1, _2, _3), random)), ep);
	}else{
		LOG << "store_node: " << ep.IP() << " " << ep.port();
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::store_node(it->second.random, local_ID)), ep);
	}
}

void kad::store_token_timeout()
{
	for(std::multimap<net::endpoint, store_token>::iterator
		it_cur = incoming_store_token.begin(); it_cur != incoming_store_token.end();)
	{
		if(it_cur->second.timeout()){
			incoming_store_token.erase(it_cur++);
		}else{
			++it_cur;
		}
	}
	for(std::multimap<net::endpoint, store_token>::iterator
		it_cur = outgoing_store_token.begin(); it_cur != outgoing_store_token.end();)
	{
		if(it_cur->second.timeout()){
			outgoing_store_token.erase(it_cur++);
		}else{
			++it_cur;
		}
	}
}

unsigned kad::upload_rate()
{
	return Exchange.upload_rate();
}
