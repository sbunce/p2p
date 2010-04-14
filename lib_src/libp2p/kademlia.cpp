#include "kademlia.hpp"

kademlia::kademlia():
	local_ID(db::table::prefs::get_ID()),
	Unknown_Reserve_Ping(0)
{
	for(int x=0; x<protocol_udp::bucket_count; ++x){
		Known_Reserve_Ping[x] = 0;
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

static bit_field ID_to_bit_field(const std::string & ID)
{
	assert(ID.size() == SHA1::hex_size);
	std::string bin = convert::hex_to_bin(ID);
	bit_field BF(reinterpret_cast<const unsigned char *>(bin.data()), bin.size(),
		protocol_udp::bucket_count);
	return BF;
}

unsigned kademlia::calc_bucket(const std::string & remote_ID)
{
	assert(remote_ID.size() == SHA1::hex_size);
	bit_field local_BF = ID_to_bit_field(local_ID);
	bit_field remote_BF = ID_to_bit_field(remote_ID);
	for(int x=protocol_udp::bucket_count - 1; x>=0; --x){
		if(remote_BF[x] != local_BF[x]){
			return x;
		}
	}
	return 0;
}

void kademlia::do_pings()
{
	//ping hosts in k_buckets that are about to time out
	for(int x=0; x<protocol_udp::bucket_count; ++x){
		std::vector<network::endpoint> tmp = Bucket[x].ping();
		for(std::vector<network::endpoint>::iterator iter_cur = tmp.begin(),
			iter_end = tmp.end(); iter_cur != iter_end; ++iter_cur)
		{
			LOGGER << iter_cur->IP() << " " << iter_cur->port();
			network::buffer random(portable_urandom(8));
			Exchange.send(boost::shared_ptr<message_udp::send::base>(
				new message_udp::send::ping(random, local_ID)), *iter_cur);
			Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
				new message_udp::recv::pong(boost::bind(&kademlia::recv_pong, this, _1, _2), random)),
				*iter_cur, boost::bind(&kademlia::timeout_ping_bucket, this, *iter_cur, x));
		}
	}

	//ping hosts in known reserve to try to fill buckets
	for(int x=0; x<protocol_udp::bucket_count; ++x){
		if(Bucket[x].size() + Known_Reserve_Ping[x] < protocol_udp::bucket_size
			&& !Known_Reserve[x].empty())
		{
			LOGGER << Known_Reserve[x].begin()->IP()
				<< " " << Known_Reserve[x].begin()->port();
			network::buffer random(portable_urandom(8));
			Exchange.send(boost::shared_ptr<message_udp::send::base>(
				new message_udp::send::ping(random, local_ID)), *Known_Reserve[x].begin());
			Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
				new message_udp::recv::pong(boost::bind(&kademlia::recv_pong_known_reserve, this, _1, _2, x), random)),
				*Known_Reserve[x].begin(), boost::bind(&kademlia::timeout_ping_known_reserve, this,
				*Known_Reserve[x].begin(), x));
			Known_Reserve[x].erase(Known_Reserve[x].begin());
			++Known_Reserve_Ping[x];
		}
	}

	//ping hosts we do not know the ID of
	if(Unknown_Reserve_Ping < 5 && !Unknown_Reserve.empty()){
		LOGGER << Unknown_Reserve.begin()->IP()
			<< " " << Unknown_Reserve.begin()->port();
		network::buffer random(portable_urandom(8));
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::ping(random, local_ID)), *Unknown_Reserve.begin());
		Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
			new message_udp::recv::pong(boost::bind(&kademlia::recv_pong_unknown_reserve, this, _1, _2), random)),
			*Unknown_Reserve.begin(), boost::bind(&kademlia::timeout_ping_unknown_reserve, this,
			*Unknown_Reserve.begin()));
		Unknown_Reserve.erase(Unknown_Reserve.begin());
		++Unknown_Reserve_Ping;
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
			LOGGER << "invalid " << tmp.back().IP << " " << tmp.back().port;
		}else{
			if(tmp.back().ID.empty()){
				Unknown_Reserve.insert(*E.begin());
			}else{
				unsigned bucket_num = calc_bucket(tmp.back().ID);
				Known_Reserve[bucket_num].insert(*E.begin());
			}
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
	LOGGER << endpoint.IP() << " " << endpoint.port() << " remote_ID: "
		<< remote_ID << " find: " << ID_to_find;
}

void kademlia::recv_ping(const network::endpoint & endpoint, const network::buffer & random,
	const std::string & remote_ID)
{
	LOGGER << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::pong(random, local_ID)), endpoint);
}

void kademlia::recv_pong(const network::endpoint & endpoint, const std::string & remote_ID)
{
	LOGGER << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	unsigned bucket_num = calc_bucket(remote_ID);
	for(int x=0; x<protocol_udp::bucket_count; ++x){
		if(Bucket[x].exists(endpoint)){
			if(x == bucket_num){
				//host updating current location in bucket
				Bucket[x].update(remote_ID, endpoint);
			}else{
				/*
				The host is responding with a different remote ID that belongs in a
				different k_bucket. Move the contact
				*/
				Bucket[x].erase(endpoint);
				if(Bucket[bucket_num].size() < protocol_udp::bucket_size){
					//room in new bucket
					Bucket[bucket_num].update(remote_ID, endpoint);
				}else{
					//no room in new bucket
					Known_Reserve[bucket_num].insert(endpoint);
				}
			}
			return;
		}
	}
	//at this point we know the contact doesn't exist in any bucket
	if(Bucket[bucket_num].size() < protocol_udp::bucket_size){
		//room in bucket
		Bucket[bucket_num].update(remote_ID, endpoint);
	}else{
		//no room in bucket
		Known_Reserve[bucket_num].insert(endpoint);
	}
}

void kademlia::recv_pong_known_reserve(const network::endpoint & endpoint,
	const std::string & remote_ID, const unsigned expected_bucket_num)
{
	assert(expected_bucket_num < protocol_udp::bucket_count);
	assert(Known_Reserve_Ping[expected_bucket_num] > 0);
	--Known_Reserve_Ping[expected_bucket_num];
	recv_pong(endpoint, remote_ID);
}

void kademlia::recv_pong_unknown_reserve(const network::endpoint & endpoint,
	const std::string & remote_ID)
{
	--Unknown_Reserve_Ping;
	recv_pong(endpoint, remote_ID);
}

void kademlia::timeout_ping_bucket(const network::endpoint endpoint,
	const unsigned bucket_num)
{
	assert(bucket_num < protocol_udp::bucket_count);
	LOGGER << endpoint.IP() << " " << endpoint.port();
	Bucket[bucket_num].erase(endpoint);
}

void kademlia::timeout_ping_known_reserve(const network::endpoint endpoint,
	const unsigned bucket_num)
{
	assert(bucket_num < protocol_udp::bucket_count);
	assert(Known_Reserve_Ping[bucket_num] > 0);
	LOGGER << endpoint.IP() << " " << endpoint.port();
	--Known_Reserve_Ping[bucket_num];
}

void kademlia::timeout_ping_unknown_reserve(const network::endpoint endpoint)
{
	assert(Unknown_Reserve_Ping > 0);
	LOGGER << endpoint.IP() << " " << endpoint.port();
	--Unknown_Reserve_Ping;
}
