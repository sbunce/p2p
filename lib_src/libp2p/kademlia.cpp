#include "kademlia.hpp"

kademlia::kademlia():
	local_ID(db::table::prefs::get_ID())
{
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		Bucket_Reserve_Ping[x] = 0;
	}

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

void kademlia::add_reserve(const std::string & remote_ID,
	const network::endpoint & endpoint)
{
	unsigned bucket_num = k_func::ID_to_bucket_num(local_ID, remote_ID);

	//don't add if endpoint already exists in another bucket or reserve bucket
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		if(Bucket[x].exists(endpoint)){
			return;
		}
		if(Bucket_Reserve[x].find(endpoint) != Bucket_Reserve[x].end()){
			return;
		}
	}

	//add new endpoint
	LOG << "reserve: " << endpoint.IP() << " " << endpoint.port();
	Bucket_Reserve[bucket_num].insert(endpoint);
}

void kademlia::do_pings()
{
	//ping hosts in Bucket that are about to time out
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		std::list<network::endpoint> tmp = Bucket[x].ping();
		for(std::list<network::endpoint>::iterator iter_cur = tmp.begin(),
			iter_end = tmp.end(); iter_cur != iter_end; ++iter_cur)
		{
			LOG << iter_cur->IP() << " " << iter_cur->port();
			network::buffer random(portable_urandom(4));
			Exchange.send(boost::shared_ptr<message_udp::send::base>(
				new message_udp::send::ping(random, local_ID)), *iter_cur);
			Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
				new message_udp::recv::pong(boost::bind(&kademlia::recv_pong, this, _1, _2), random)),
				*iter_cur, boost::bind(&kademlia::timeout_ping_bucket, this, *iter_cur, x));
		}
	}

	//ping hosts in Bucket_Reserve to try to fill buckets
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		if(Bucket[x].size() + Bucket_Reserve_Ping[x] < protocol_udp::bucket_size
			&& !Bucket_Reserve[x].empty())
		{
			LOG << Bucket_Reserve[x].begin()->IP()
				<< " " << Bucket_Reserve[x].begin()->port();
			network::buffer random(portable_urandom(4));
			Exchange.send(boost::shared_ptr<message_udp::send::base>(
				new message_udp::send::ping(random, local_ID)), *Bucket_Reserve[x].begin());
			Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
				new message_udp::recv::pong(boost::bind(&kademlia::recv_pong_bucket_reserve, this, _1, _2, x), random)),
				*Bucket_Reserve[x].begin(), boost::bind(&kademlia::timeout_ping_bucket_reserve, this,
				*Bucket_Reserve[x].begin(), x));
			Bucket_Reserve[x].erase(Bucket_Reserve[x].begin());
			++Bucket_Reserve_Ping[x];
		}
	}
}

void kademlia::main_loop()
{
	//read in hosts from database
	std::list<db::table::peer::info> tmp = db::table::peer::resume();
	while(!tmp.empty()){
		std::set<network::endpoint> E = network::get_endpoint(tmp.back().IP,
			tmp.back().port, network::udp);
		if(E.empty()){
			LOG << "failed \"" << tmp.back().IP << "\" " << tmp.back().port;
		}else{
			unsigned bucket_num = k_func::ID_to_bucket_num(local_ID, tmp.back().ID);
			Bucket_Reserve[bucket_num].insert(*E.begin());
		}
		tmp.pop_back();
	}

	//main loop
	std::time_t last_time(std::time(NULL));
	while(true){
		boost::this_thread::interruption_point();
		if(last_time != std::time(NULL)){
			//once per second
			do_pings();
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
	add_reserve(remote_ID, endpoint);

	//get up to protocol_udp::bucket_size hosts which are closer
	std::map<mpa::mpint, std::pair<std::string, network::endpoint> > hosts;
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		Bucket[x].find_node(ID_to_find, hosts);
	}

	//calculate buckets the remote host should expect to put the hosts in
	std::list<std::pair<network::endpoint, unsigned char> > hosts_final;
	for(std::map<mpa::mpint, std::pair<std::string, network::endpoint> >::iterator
		iter_cur = hosts.begin(), iter_end = hosts.end(); iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->first == ID_to_find){
			hosts_final.push_back(std::make_pair(iter_cur->second.second, 255));
		}else{
			hosts_final.push_back(std::make_pair(iter_cur->second.second,
				k_func::ID_to_bucket_num(remote_ID, iter_cur->second.first)));
		}
	}

	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::host_list(random, local_ID, hosts_final)), endpoint);
}

void kademlia::recv_ping(const network::endpoint & endpoint, const network::buffer & random,
	const std::string & remote_ID)
{
	LOG << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	add_reserve(remote_ID, endpoint);
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::pong(random, local_ID)), endpoint);
}

void kademlia::recv_pong(const network::endpoint & endpoint, const std::string & remote_ID)
{
	unsigned bucket_num = k_func::ID_to_bucket_num(local_ID, remote_ID);
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		if(Bucket[x].exists(endpoint)){
			if(x == bucket_num){
				//host updating current location in bucket
				LOG << "update: " << endpoint.IP() << " " << endpoint.port()
					<< " " << remote_ID;
				Bucket[x].update(remote_ID, endpoint);
			}else{
				/*
				The host is responding with a different remote ID that belongs in a
				different k_bucket. Move the contact
				*/
				Bucket[x].erase(endpoint);
				if(Bucket[bucket_num].size() < protocol_udp::bucket_size){
					//room in new bucket
					LOG << "move_add: " << endpoint.IP() << " " << endpoint.port()
						<< " " << remote_ID;
					Bucket[bucket_num].update(remote_ID, endpoint);
				}else{
					//no room in new bucket
					LOG << "move_reserve: " << endpoint.IP() << " " << endpoint.port()
						<< " " << remote_ID;
					Bucket_Reserve[bucket_num].insert(endpoint);
				}
			}
			return;
		}
	}
	//at this point we know the contact doesn't exist in any bucket
	if(Bucket[bucket_num].size() < protocol_udp::bucket_size){
		//room in bucket
		LOG << "add: " << endpoint.IP() << " " << endpoint.port()
			<< " " << remote_ID;
		Bucket[bucket_num].update(remote_ID, endpoint);
	}else{
		//no room in bucket
		LOG << "reserve: " << endpoint.IP() << " " << endpoint.port()
			<< " " << remote_ID;
		Bucket_Reserve[bucket_num].insert(endpoint);
	}
}

void kademlia::recv_pong_bucket_reserve(const network::endpoint & endpoint,
	const std::string & remote_ID, const unsigned expected_bucket_num)
{
	assert(expected_bucket_num < protocol_udp::bucket_count);
	--Bucket_Reserve_Ping[expected_bucket_num];
	recv_pong(endpoint, remote_ID);
}

void kademlia::timeout_ping_bucket(const network::endpoint endpoint,
	const unsigned bucket_num)
{
	assert(bucket_num < protocol_udp::bucket_count);
	LOG << endpoint.IP() << " " << endpoint.port();
	Bucket[bucket_num].erase(endpoint);
}

void kademlia::timeout_ping_bucket_reserve(const network::endpoint endpoint,
	const unsigned bucket_num)
{
	assert(bucket_num < protocol_udp::bucket_count);
	LOG << endpoint.IP() << " " << endpoint.port();
	--Bucket_Reserve_Ping[bucket_num];
}
