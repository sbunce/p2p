#include "kademlia.hpp"

kademlia::kademlia():
	local_ID(database::table::prefs::get_ID())
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
		kademlia::bucket_count);
	return BF;
}

unsigned kademlia::calc_bucket(const std::string & remote_ID)
{
	assert(remote_ID.size() == SHA1::hex_size);
	bit_field local_BF = ID_to_bit_field(local_ID);
	bit_field remote_BF = ID_to_bit_field(remote_ID);
	for(int x=bucket_count - 1; x>=0; --x){
		if(remote_BF[x] != local_BF[x]){
			return x;
		}
	}
	return 0;
}

void kademlia::main_loop()
{
	//fill bucket cache
	std::vector<database::table::peer::info> tmp = database::table::peer::resume();
	//randomize so we put different contacts in k-buckets each time
	std::random_shuffle(tmp.begin(), tmp.end());
/*
	while(!tmp.empty()){
		std::set<network::endpoint> E = network::get_endpoint(tmp.back().IP,
			tmp.back().port, network::udp);
		if(E.empty()){
			LOGGER << "invalid " << tmp.back().IP << " " << tmp.back().port;
		}else{
			int bucket_num = calc_bucket(tmp.back().ID);
			Bucket_Cache[bucket_num].push_back(contact(tmp.back().ID, *E.begin()));
		}
		tmp.pop_back();
	}
*/
	std::time_t last_time(std::time(NULL));
	while(true){
		boost::this_thread::interruption_point();
		if(last_time != std::time(NULL)){
			//once per second
			process_bucket_cache();
			last_time = std::time(NULL);
		}
		Exchange.recv();
	}
}

void kademlia::process_bucket_cache()
{
/*
	//give each cached contact and equal chance to be added to a k-bucket
	for(unsigned x=(bucket_cache_latest + 1) % bucket_count;
		x != bucket_cache_latest; x = (x + 1) % bucket_count)
	{
		if(Bucket[x].size() < max_bucket_size){
			for(std::list<contact>::iterator iter_cur = Bucket_Cache[x].begin(),
				iter_end = Bucket_Cache[x].end(); iter_cur != iter_end; ++iter_cur)
			{
				if(!iter_cur->ping_sent){
					LOGGER << "ping " << iter_cur->endpoint.IP() << " "
						<< iter_cur->endpoint.port();
					network::buffer random(portable_urandom(8));
					Exchange.send(boost::shared_ptr<message_udp::send::base>(
						new message_udp::send::ping(random, local_ID)), iter_cur->endpoint);
					Exchange.expect_response(boost::shared_ptr<message_udp::recv::base>(
						new message_udp::recv::pong(boost::bind(&kademlia::recv_pong,
						this, _1), random)));
					iter_cur->ping_sent = true;
					bucket_cache_latest = x;
				}
			}
		}
	}
*/
}

void kademlia::recv_ping(const network::buffer & random, const std::string & remote_ID)
{
LOGGER << remote_ID;
//DEBUG, update k-bucket here

//DEBUG, need IP to send to
/*
	Exchange.send(boost::shared_ptr<message_udp::send::base>(
		new message_udp::send::pong(random, local_ID)));
*/
}

void kademlia::recv_pong(const std::string & remote_ID)
{
LOGGER << remote_ID;
//DEBUG, update k-bucket here

}
