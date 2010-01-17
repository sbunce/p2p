#include "connection.hpp"

connection::connection(
	network::proactor & Proactor_in,
	network::connection_info & CI
):
	Exchange(Mutex, Proactor_in, CI),
	Slot_Manager(Exchange)
{
	send_initial();
}

bool connection::empty()
{
	boost::mutex::scoped_lock lock(Mutex);
	return Slot_Manager.empty();
}

bool connection::recv_initial(boost::shared_ptr<message::base> M)
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

void connection::send_initial()
{
	std::string peer_ID = database::table::prefs::get_peer_ID();
	Exchange.send(boost::shared_ptr<message::base>(new message::initial(peer_ID)));
	Exchange.expect_response(boost::shared_ptr<message::base>(new message::initial(
		boost::bind(&connection::recv_initial, this, _1))));
}
