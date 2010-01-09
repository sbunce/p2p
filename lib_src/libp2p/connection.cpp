#include "connection.hpp"

connection::connection(
	network::proactor & Proactor_in,
	network::connection_info & CI
):
	Exchange(Proactor_in, CI),
	Slot_Manager(Exchange)
{
	CI.recv_call_back = boost::bind(&connection::recv_call_back, this, _1);
	send_initial();
}

void connection::recv_call_back(network::connection_info & CI)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	/*
	We have this function as a wrapper for exchange::recv_call_back so we can
	lock the mutex that needs to be locked for all entry points to the connection
	object.
	*/
	Exchange.recv_call_back(CI);
}

bool connection::recv_initial(boost::shared_ptr<message::base> M)
{
	std::string peer_ID = convert::bin_to_hex(std::string(
		reinterpret_cast<const char *>(M->buf.data()), SHA1::bin_size));
	LOGGER << peer_ID;
	Slot_Manager.resume(peer_ID);
	return true;
}

void connection::send_initial()
{
	std::string peer_ID = database::table::prefs::get_peer_ID();
	Exchange.send(boost::shared_ptr<message::base>(new message::initial(peer_ID)));
	Exchange.expect_response(boost::shared_ptr<message::base>(new message::initial(
		boost::bind(&connection::recv_initial, this, _1))));
}
