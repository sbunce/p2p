#include "connection.hpp"

connection::connection(
	net::proactor & Proactor_in,
	net::connection_info & CI,
	const boost::function<void(const net::endpoint & ep, const std::string & hash)> & peer_call_back,
	const boost::function<void(const int)> & trigger_tick
):
	Exchange(Proactor_in, CI),
	Slot_Manager(Exchange, peer_call_back, trigger_tick),
	remote_ep(CI.ep)
{
	CI.recv_call_back = boost::bind(&connection::recv_call_back, this, _1);
	CI.send_call_back = boost::bind(&connection::send_call_back, this, _1);

	//send initial messages
	Exchange.send(boost::shared_ptr<message_tcp::send::base>(
		new message_tcp::send::initial_ID(db::table::prefs::get_ID())));
	if(CI.direction == net::outgoing){
		/*
		The remote host will only see our randomly bound port when we connect to
		it. We need to tell it what port it can contact us on.
		*/
		Exchange.send(boost::shared_ptr<message_tcp::send::base>(
			new message_tcp::send::initial_port(db::table::prefs::get_port())));
	}

	//expect initial messages
	if(CI.direction == net::incoming){
		Exchange.expect_response(boost::shared_ptr<message_tcp::recv::base>(
			new message_tcp::recv::initial_ID(boost::bind(&connection::recv_initial_ID,
			this, _1))));
		//remote host will tell us what port we can contact it on
		Exchange.expect_response(boost::shared_ptr<message_tcp::recv::base>(
			new message_tcp::recv::initial_port(boost::bind(&connection::recv_initial_port,
			this, _1))));
	}else{
		Exchange.expect_response(boost::shared_ptr<message_tcp::recv::base>(
			new message_tcp::recv::initial_ID(boost::bind(&connection::recv_initial_ID,
			this, _1))));
		Slot_Manager.set_remote_listen(CI.ep);
	}
}

void connection::add(const std::string & hash)
{
	Slot_Manager.add(hash);
}

bool connection::empty()
{
	boost::mutex::scoped_lock lock(Mutex);
	return Slot_Manager.empty();
}

void connection::recv_call_back(net::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Mutex);
	Exchange.recv_call_back(CI);
	Slot_Manager.tick();
}

bool connection::recv_initial_ID(const std::string & remote_ID)
{
//DEBUG, need to make sure ID is what we expected
	LOG << convert::abbr(remote_ID);
	return true;
}

bool connection::recv_initial_port(const std::string & port)
{
	LOG << port;
	boost::optional<net::endpoint> remote_listen = net::bin_to_endpoint(remote_ep.IP_bin(),
		convert::int_to_bin(boost::lexical_cast<boost::uint16_t>(port)));
	assert(remote_listen);
	assert(remote_listen->port() == port);
	Slot_Manager.set_remote_listen(*remote_listen);
	return true;
}

void connection::remove(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	Slot_Manager.remove(hash);
}

void connection::send_call_back(net::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Mutex);
	Exchange.send_call_back(CI);
}

void connection::tick()
{
	boost::mutex::scoped_lock lock(Mutex);
	Slot_Manager.tick();
}
