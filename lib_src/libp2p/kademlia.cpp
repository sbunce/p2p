#include "kademlia.hpp"

kademlia::kademlia():
	local_ID(database::table::prefs::get_ID()),
	Known_Reserve_Ping(protocol_udp::bucket_count)
{
	//messages to expect anytime
	Exchange.expect_anytime(boost::shared_ptr<message_udp::recv::base>(
		new message_udp::recv::ping(boost::bind(&kademlia::recv_ping, this, _1, _2))));

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
	std::set<network::endpoint> ping_set;
	for(int x=0; x<protocol_udp::bucket_count; ++x){
		std::vector<network::endpoint> tmp = Bucket[x].ping();
		ping_set.insert(tmp.begin(), tmp.end());
	}
	for(std::set<network::endpoint>::iterator iter_cur = ping_set.begin(),
		iter_end = ping_set.end(); iter_cur != iter_end; ++iter_cur)
	{
		LOGGER << "ping " << iter_cur->IP() << " " << iter_cur->port();
		network::buffer random(portable_urandom(8));
		Exchange.send(boost::shared_ptr<message_udp::send::base>(
			new message_udp::send::ping(random)), *iter_cur);
		Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
			new message_udp::recv::pong(boost::bind(&kademlia::recv_pong, this, _1, _2),
			random)), *iter_cur);
	}

//DEBUG,
	//do pings not originating from k_buckets
}

bool kademlia::endpoint_exists_in_k_bucket(const network::endpoint & endpoint)
{
	for(int x=0; x<protocol_udp::bucket_count; ++x){
		if(Bucket[x].exists(endpoint)){
			return true;
		}
	}
	return false;
}

bool kademlia::endpoint_exists_in_reserve(const network::endpoint & endpoint)
{
	for(int x=0; x<protocol_udp::bucket_count; ++x){
		if(Known_Reserve[x].find(endpoint) != Known_Reserve[x].end()){
			return true;
		}
	}
	if(Unknown_Reserve.find(endpoint) != Unknown_Reserve.end()){
		return true;
	}
	return false;
}

void kademlia::main_loop()
{
	std::list<database::table::peer::info> tmp = database::table::peer::resume();

//DEBUG, will fail to respond.
tmp.push_back(database::table::peer::info("", "192.168.1.169", "6969"));

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

void kademlia::recv_ping(const network::endpoint & endpoint, const network::buffer & random)
{
	LOGGER << endpoint.IP() << " " << endpoint.port();
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::pong(random, local_ID)), endpoint);
}

void kademlia::recv_pong(const network::endpoint & endpoint, const std::string & remote_ID)
{
	LOGGER << endpoint.IP() << " " << endpoint.port() << " " << remote_ID;
	unsigned bucket_num = calc_bucket(remote_ID);
	if(!Bucket[bucket_num].update(remote_ID, endpoint)){
		//bucket not updated, save endpoint for later
		Known_Reserve[bucket_num].insert(endpoint);
	}
}

void kademlia::timeout_ping(const network::endpoint endpoint)
{

}
