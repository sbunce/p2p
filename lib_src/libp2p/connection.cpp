#include "connection.hpp"

connection::connection(
	net::proactor & Proactor_in,
	net::proactor::connection_info & CI,
	const boost::function<void(const int)> & trigger_tick
):
	Exchange(Proactor_in, CI),
	Slot_Manager(Exchange, trigger_tick),
	remote_ep(CI.ep)
{
	CI.recv_call_back = boost::bind(&connection::recv_call_back, this, _1);
	CI.send_call_back = boost::bind(&connection::send_call_back, this, _1);
	send_initial();
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

void connection::recv_call_back(net::proactor::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Mutex);
	Exchange.recv_call_back(CI);
	Slot_Manager.tick();
}

bool connection::recv_initial(const std::string & remote_ID,
	const std::string & port)
{
//DEBUG, need to make sure ID is what we expected
	LOG << convert::abbr(remote_ID) << " " << port;

	boost::uint16_t port_int = boost::lexical_cast<boost::uint16_t>(port);
LOG << port_int;
LOG << convert::bin_to_int<boost::uint16_t>(convert::int_to_bin(port_int));

	boost::optional<net::endpoint> remote_listen = net::bin_to_endpoint(remote_ep.IP_bin(),
		convert::int_to_bin(port_int));
	assert(remote_listen);
LOG << remote_listen->IP() << " " << remote_listen->port();
	assert(remote_listen->port() == port);
	Slot_Manager.set_remote_listen(*remote_listen);
	return true;
}

void connection::remove(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	Slot_Manager.remove(hash);
}

void connection::send_call_back(net::proactor::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Mutex);
	Exchange.send_call_back(CI);
}

void connection::send_initial()
{
	std::string ID = db::table::prefs::get_ID();
	Exchange.send(boost::shared_ptr<message_tcp::send::base>(
		new message_tcp::send::initial(ID, db::table::prefs::get_port())));
	Exchange.expect_response(boost::shared_ptr<message_tcp::recv::base>(
		new message_tcp::recv::initial(boost::bind(&connection::recv_initial,
		this, _1, _2))));
}

void connection::tick()
{
	boost::mutex::scoped_lock lock(Mutex);
	Slot_Manager.tick();
}
