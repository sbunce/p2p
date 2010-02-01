#include "kademlia.hpp"

kademlia::kademlia()
{
	network_thread = boost::thread(boost::bind(&kademlia::network_loop, this));
}

kademlia::~kademlia()
{
	network_thread.interrupt();
	network_thread.join();
}

void kademlia::network_loop()
{
	//setup UDP listener
	std::set<network::endpoint> E = network::get_endpoint(
		"",
		database::table::prefs::get_port(),
		network::udp
	);
	assert(!E.empty());
	network::ndgram ndgram(*E.begin());
	assert(ndgram.is_open());

	//setup select
	network::select select;
	std::set<int> read, tmp_read, empty;
	read.insert(ndgram.socket());

	while(true){
		boost::this_thread::interruption_point();
		tmp_read = read;
		select(tmp_read, empty, 1000);
		if(tmp_read.empty()){
			//nothing received
			continue;
		}

		//receive message
		boost::shared_ptr<network::endpoint> from;
		network::buffer buf;
		ndgram.recv(buf, from);
		assert(from);

		LOGGER << "from " << from->IP() << " " << from->port() << " " << buf;
	}
}
