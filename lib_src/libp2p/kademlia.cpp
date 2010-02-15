#include "kademlia.hpp"

//BEGIN contact
kademlia::contact::contact(
	const std::string & ID_in,
	const std::string & IP_in,
	const std::string & port_in
):
	ID(ID_in),
	IP(IP_in),
	port(port_in),
	last_seen(std::time(NULL)),
	ping_sent(false)
{

}

kademlia::contact::contact(const kademlia::contact & C):
	ID(C.ID),
	IP(C.IP),
	port(C.port),
	last_seen(C.last_seen),
	ping_sent(C.ping_sent)
{

}

//returns true if a ping needs to be sent
bool kademlia::contact::do_ping()
{
	//allow 60s to receive pong
	if(!ping_sent && std::time(NULL) - last_seen > 3540){
		ping_sent = true;
		return true;
	}else{
		return false;
	}
}

//returns true if contact has timed out and needs to be removed
bool kademlia::contact::timed_out()
{
	return std::time(NULL) - last_seen > 3600;
}

//updates the last seen time
void kademlia::contact::touch()
{
	last_seen = std::time(NULL);
	ping_sent = false;
}
//END contact

kademlia::kademlia()
{
	network_thread = boost::thread(boost::bind(&kademlia::network_loop, this));
}

kademlia::~kademlia()
{
	network_thread.interrupt();
	network_thread.join();
}

void kademlia::add_node(const std::string & ID, const std::string & IP,
	const std::string & port)
{
	boost::mutex::scoped_lock lock(high_node_cache_mutex);
	high_node_cache.push_back(database::table::peer::info(ID, IP, port));
}

unsigned calc_bucket(const std::string & ID)
{
	assert(ID.size() == SHA1::hex_size);
}

void kademlia::network_loop()
{
	/*
	Read nodes from database to try to put in k-buckets. Randomize them so we
	use different nodes on each program start.
	*/
	low_node_cache = database::table::peer::resume();
	std::random_shuffle(low_node_cache.begin(), low_node_cache.end());

	//setup UDP listener
	std::set<network::endpoint> E = network::get_endpoint(
		"",
		database::table::prefs::get_port(),
		network::udp
	);
	assert(!E.empty());
	ndgram.open(*E.begin());
	assert(ndgram.is_open());

	//setup select to wait on UDP listener
	std::set<int> read, tmp_read, empty;
	read.insert(ndgram.socket());
	network::select select;

	std::time_t last_time(std::time(NULL));
	while(true){
		boost::this_thread::interruption_point();

		if(last_time != std::time(NULL)){
			//stuff to do once per second
			process_node_cache();
			last_time = std::time(NULL);
		}

		//receive max of 10 messages per second
		boost::this_thread::sleep(boost::posix_time::milliseconds(100));

		//poll socket
		tmp_read = read;
		select(tmp_read, empty, 0);
		if(tmp_read.empty()){
			//nothing received
			continue;
		}

		//receive message
		boost::shared_ptr<network::endpoint> from;
		network::buffer buf;
		ndgram.recv(buf, from);
		assert(from);

		LOGGER << "kad " << from->IP() << " " << from->port();
	}
}

void kademlia::process_node_cache()
{

}
