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

void kademlia::do_pings()
{
	//ping hosts in Bucket that are about to time out
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		std::vector<network::endpoint> tmp = Bucket[x].ping();
		for(std::vector<network::endpoint>::iterator iter_cur = tmp.begin(),
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

	//ping hosts in Bucket_Overflow to try to fill buckets
	for(unsigned x=0; x<protocol_udp::bucket_count; ++x){
		if(Bucket[x].size() + Bucket_Overflow_Ping[x] < protocol_udp::bucket_size
			&& !Bucket_Overflow[x].empty())
		{
			LOG << Bucket_Overflow[x].begin()->IP()
				<< " " << Bucket_Overflow[x].begin()->port();
			network::buffer random(portable_urandom(4));
			Exchange.send(boost::shared_ptr<message_udp::send::base>(
				new message_udp::send::ping(random, local_ID)), *Bucket_Overflow[x].begin());
			Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
				new message_udp::recv::pong(boost::bind(&kademlia::recv_pong_bucket_overflow, this, _1, _2, x), random)),
				*Bucket_Overflow[x].begin(), boost::bind(&kademlia::timeout_ping_bucket_overflow, this,
				*Bucket_Overflow[x].begin(), x));
			Bucket_Overflow[x].erase(Bucket_Overflow[x].begin());
			++Bucket_Overflow_Ping[x];
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
			Bucket_Overflow[bucket_num].insert(*E.begin());
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

	std::list<std::pair<network::endpoint, unsigned char> > hosts;


	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::host_list(random, local_ID, hosts)), endpoint);

/*
	host_list(const network::buffer & random, const std::string & local_ID,
		const std::list<std::pair<network::endpoint, unsigned char> > & hosts);
*/
}

void kademlia::recv_ping(const network::endpoint & endpoint, const network::buffer & random,
	const std::string & remote_ID)
{
	LOG << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::pong(random, local_ID)), endpoint);
}

void kademlia::recv_pong(const network::endpoint & endpoint, const std::string & remote_ID)
{
	LOG << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	unsigned bucket_num = k_func::ID_to_bucket_num(local_ID, remote_ID);
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
					Bucket_Overflow[bucket_num].insert(endpoint);
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
		Bucket_Overflow[bucket_num].insert(endpoint);
	}
}

void kademlia::recv_pong_bucket_overflow(const network::endpoint & endpoint,
	const std::string & remote_ID, const unsigned expected_bucket_num)
{
	assert(expected_bucket_num < protocol_udp::bucket_count);
	--Bucket_Overflow_Ping[expected_bucket_num];
	recv_pong(endpoint, remote_ID);
}

void kademlia::timeout_ping_bucket(const network::endpoint endpoint,
	const unsigned bucket_num)
{
	assert(bucket_num < protocol_udp::bucket_count);
	LOG << endpoint.IP() << " " << endpoint.port();
	Bucket[bucket_num].erase(endpoint);
}

void kademlia::timeout_ping_bucket_overflow(const network::endpoint endpoint,
	const unsigned bucket_num)
{
	assert(bucket_num < protocol_udp::bucket_count);
	LOG << endpoint.IP() << " " << endpoint.port();
	--Bucket_Overflow_Ping[bucket_num];
}
