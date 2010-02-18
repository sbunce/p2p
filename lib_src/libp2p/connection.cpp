#include "connection.hpp"

connection::connection(
	network::proactor & Proactor_in,
	network::connection_info & CI,
	boost::function<void(const int)> trigger_tick
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

void connection::recv_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Mutex);
	Exchange.recv_call_back(CI);
	Slot_Manager.tick();
}

bool connection::recv_initial(boost::shared_ptr<message_tcp::base> M)
{
	std::string peer_ID = convert::bin_to_hex(std::string(
		reinterpret_cast<const char *>(M->buf.data()), SHA1::bin_size));
	LOGGER << peer_ID;
	Slot_Manager.resume(peer_ID);
	return true;
}

void connection::remove(const std::string & hash)
{
	boost::mutex::scoped_lock lock(Mutex);
	Slot_Manager.remove(hash);
}

void connection::send_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Mutex);
	Exchange.send_call_back(CI);
}

void connection::send_initial()
{
	std::string ID = database::table::prefs::get_ID();
	Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::initial(ID)));
	Exchange.expect_response(boost::shared_ptr<message_tcp::base>(new message_tcp::initial(
		boost::bind(&connection::recv_initial, this, _1))));
}

void connection::tick()
{
	boost::mutex::scoped_lock lock(Mutex);
	Slot_Manager.tick();
}
