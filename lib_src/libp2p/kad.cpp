#include "kad.hpp"

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
	Exchange.expect_anytime(boost::shared_ptr<message_udp::recv::base>(
		new message_udp::recv::store_file(boost::bind(&kad::recv_store_file,
		this, _1, _2, _3, _4))));
	Exchange.expect_anytime(boost::shared_ptr<message_udp::recv::base>(
		new message_udp::recv::query_file(boost::bind(&kad::recv_query_file,
		this, _1, _2, _3, _4))));

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

void kad::find_file(const std::string & hash,
	const boost::function<void (const net::endpoint &)> & call_back)
{
	boost::mutex::scoped_lock lock(relay_job_mutex);
	relay_job.push_back(boost::bind(&kad::find_file_relay, this, hash, call_back));
}

void kad::find_file_relay(const std::string hash,
	const boost::function<void (const net::endpoint &)> call_back)
{
	/*
	It is possible (and likely) that multiple hosts will send some of the same
	node IDs in node_lists. Memoize node IDs that arrive in node_lists so we
	don't start multiple k_find::set jobs for the same node ID.
	*/
	boost::shared_ptr<std::set<std::string> > node_list_memoize(new std::set<std::string>());

	//find closest nodes to hash
	std::multimap<mpa::mpint, net::endpoint> hosts = Route_Table.find_node_local(hash);
	Find.set(hash, hosts, boost::bind(&kad::find_file_call_back_0, this, _1,
		hash, call_back, node_list_memoize));
}

void kad::find_file_call_back_0(const net::endpoint & ep,
	const std::string hash,
	const boost::function<void (const net::endpoint &)> call_back,
	boost::shared_ptr<std::set<std::string> > node_list_memoize)
{
	LOG << ep.IP() << " " << ep.port() << " " << convert::abbr(hash);
	//ask closest nodes what nodes have the file
	net::buffer random(random::urandom(4));
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::query_file(random, local_ID, hash)), ep);
	Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
		new message_udp::recv::node_list(boost::bind(&kad::find_file_call_back_1,
		this, _1, _2, _3, _4, call_back, node_list_memoize), random)), ep);
}

void kad::find_file_call_back_1(const net::endpoint & from,
	const net::buffer & random, const std::string & remote_ID,
	const std::list<std::string> & nodes,
	const boost::function<void (const net::endpoint &)> call_back,
	boost::shared_ptr<std::set<std::string> > node_list_memoize)
{
	LOG << from.IP() << " " << from.port();
	//find address of node with file
	for(std::list<std::string>::const_iterator it_cur = nodes.begin(),
		it_end = nodes.end(); it_cur != it_end; ++it_cur)
	{
		if(node_list_memoize->find(*it_cur) == node_list_memoize->end()){
			node_list_memoize->insert(*it_cur);
			std::multimap<mpa::mpint, net::endpoint> hosts = Route_Table.find_node_local(*it_cur);
			Find.set(*it_cur, hosts, call_back);
		}
	}
}

void kad::find_node(const std::string & ID,
	const boost::function<void (const net::endpoint &)> & call_back)
{
	boost::mutex::scoped_lock lock(relay_job_mutex);
	relay_job.push_back(boost::bind(&kad::find_node_relay, this, ID, call_back));
}

void kad::find_node_relay(const std::string ID,
	const boost::function<void (const net::endpoint &)> call_back)
{
	std::multimap<mpa::mpint, net::endpoint> hosts = Route_Table.find_node_local(ID);
	Find.node(ID, hosts, call_back);
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
			Find.tick();
			Route_Table.tick();
			Token.tick();
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
	LOG << from.IP() << " " << from.port() << " find: " << convert::abbr(ID_to_find);
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
	LOG << from.IP() << " " << from.port() << " " << convert::abbr(remote_ID);
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
	Token.issue(from, random);
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::pong(random, local_ID)), from);
}

void kad::recv_pong(const net::endpoint & from, const net::buffer & random,
	const std::string & remote_ID)
{
	Token.receive(from, random);
	Route_Table.recv_pong(from, remote_ID);
}

void kad::recv_query_file(const net::endpoint & from, const net::buffer & random,
	const std::string & remote_ID, const std::string & hash)
{
	LOG << from.IP() << " " << from.port() << " " << convert::abbr(hash);
	Route_Table.add_reserve(from, remote_ID);
	std::list<std::string> nodes = db::table::source::get_ID(hash);
	//randomize what nodes we send remote host
	std::vector<std::string> vec(nodes.begin(), nodes.end());
	nodes.clear();
	std::random_shuffle(vec.begin(), vec.end());
	for(std::vector<std::string>::iterator it_cur = vec.begin(),
		it_end = vec.end(); it_cur != it_end && nodes.size() < protocol_udp::node_list_elements;
		++it_cur)
	{
		nodes.push_back(*it_cur);
	}
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::node_list(random, local_ID, nodes)), from);
}

void kad::recv_store_file(const net::endpoint & from, const net::buffer & random,
	const std::string & remote_ID, const std::string & hash)
{
	Route_Table.add_reserve(from, remote_ID);
	if(Token.has_been_issued(from, random)){
		LOG << from.IP() << " " << from.port() << " " << convert::abbr(remote_ID)
			<< " " << convert::abbr(hash);
		db::table::source::add(remote_ID, hash);
	}else{
		LOG << "invalid token: " << from.IP() << " " << from.port() << " " << convert::abbr(hash);
	}
}

void kad::recv_store_node(const net::endpoint & from, const net::buffer & random,
	const std::string & remote_ID)
{
	if(Token.has_been_issued(from, random)){
		LOG << from.IP() << " " << from.port() << " " << convert::abbr(remote_ID);
		db::table::peer::add(db::table::peer::info(remote_ID, from.IP(), from.port()));
	}else{
		LOG << "invalid token: " << from.IP() << " " << from.port() << convert::abbr(remote_ID);
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
	std::multimap<mpa::mpint, net::endpoint> hosts = Route_Table.find_node_local(local_ID);
	Find.set(local_ID, hosts, boost::bind(&kad::send_store_node_call_back_0, this, _1));
}

void kad::send_store_node_call_back_0(const net::endpoint & ep)
{
	boost::optional<net::buffer> random = Token.get_token(ep);
	if(random){
		//use existing token
		LOG << "store_node: " << ep.IP() << " " << ep.port();
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::store_node(*random, local_ID)), ep);
	}else{
		//get token
		LOG << "ping: " << ep.IP() << " " << ep.port();
		net::buffer random(random::urandom(4));
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::ping(random, local_ID)), ep);
		Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
			new message_udp::recv::pong(boost::bind(&kad::send_store_node_call_back_1,
			this, _1, _2, _3), random)), ep);
	}
}

void kad::send_store_node_call_back_1(const net::endpoint & from,
	const net::buffer & random, const std::string & remote_ID)
{
	recv_pong(from, random, remote_ID);
	LOG << "store_node: " << from.IP() << " " << from.port();
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::store_node(random, local_ID)), from);
}

void kad::store_file(const std::string & hash)
{
	boost::mutex::scoped_lock lock(relay_job_mutex);
	relay_job.push_back(boost::bind(&kad::store_file_relay, this, hash));
}

void kad::store_file_relay(const std::string hash)
{
	std::multimap<mpa::mpint, net::endpoint> hosts = Route_Table.find_node_local(local_ID);
	Find.set(local_ID, hosts, boost::bind(&kad::store_file_call_back_0, this, _1, hash));
}

void kad::store_file_call_back_0(const net::endpoint & ep, const std::string hash)
{
	boost::optional<net::buffer> random = Token.get_token(ep);
	if(random){
		//use existing token
		LOG << "store_file: " << ep.IP() << " " << ep.port() << " " << convert::abbr(hash);
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::store_file(*random, local_ID, hash)), ep);
	}else{
		//get token
		LOG << "ping: " << ep.IP() << " " << ep.port();
		net::buffer random(random::urandom(4));
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::ping(random, local_ID)), ep);
		Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
			new message_udp::recv::pong(boost::bind(&kad::store_file_call_back_1,
			this, _1, _2, _3, hash), random)), ep);
	}
}

void kad::store_file_call_back_1(const net::endpoint & from,
	const net::buffer & random, const std::string & remote_ID,
	const std::string hash)
{
	recv_pong(from, random, remote_ID);
	LOG << "store_node: " << from.IP() << " " << from.port() << " " << convert::abbr(hash);
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::store_file(random, local_ID, hash)), from);
}

unsigned kad::upload_rate()
{
	return Exchange.upload_rate();
}
