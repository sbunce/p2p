#include "connection.hpp"

connection::connection(
	net::proactor & Proactor_in,
	net::proactor::connection_info & CI,
	const boost::function<void(const int)> & trigger_tick
):
	Exchange(Proactor_in, CI),
	Slot_Manager(Exchange, trigger_tick)
{
	CI.recv_call_back = boost::bind(&connection::recv_call_back, this, _1);
	CI.send_call_back = boost::bind(&connection::send_call_back, this, _1);
	send_initial();
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

bool connection::recv_initial(const std::string & ID)
{
	LOG << convert::abbr(ID);
	Slot_Manager.resume(ID);
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
	Exchange.send(boost::shared_ptr<message_tcp::send::base>(new message_tcp::send::initial(ID)));
	Exchange.expect_response(boost::shared_ptr<message_tcp::recv::base>(new message_tcp::recv::initial(
		boost::bind(&connection::recv_initial, this, _1))));
}

void connection::tick()
{
	boost::mutex::scoped_lock lock(Mutex);
	Slot_Manager.tick();
}
