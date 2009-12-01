#include "slot_manager.hpp"

slot_manager::slot_manager(
	exchange & Exchange_in,
	network::connection_info & CI
):
	Exchange(Exchange_in),
	share_slot_state(0)
{

}

void slot_manager::make_slot_requests()
{
	while(!Pending_Slot_Request.empty() && Outgoing_Slot.size() < 256){
		boost::shared_ptr<exchange::message> M(new exchange::message());
		M->send_buf.append(protocol::REQUEST_SLOT)
			.append(convert::hex_to_bin(Pending_Slot_Request.front()->hash()));
		M->expected_response.push_back(std::make_pair(protocol::SLOT_ID,
			protocol::SLOT_ID_SIZE));
		M->expected_response.push_back(std::make_pair(protocol::REQUEST_SLOT_FAILED,
			protocol::REQUEST_SLOT_FAILED_SIZE));
		Exchange.schedule_send(M);
		Slot_Request.push_back(Pending_Slot_Request.front());
		Pending_Slot_Request.pop_front();
	}
}

bool slot_manager::recv_REQUEST_SLOT(boost::shared_ptr<exchange::message> & M)
{
	if(Incoming_Slot.size() > 256){
		return false;
	}

	//see if we have a file with the requested hash
	std::string hash = convert::bin_to_hex(std::string(
		reinterpret_cast<const char *>(M->recv_buf.data()+1), SHA1::bin_size));
	share::slot_iterator slot_iter = share::singleton().find_slot(hash);
	if(slot_iter == share::singleton().end_slot()){
		LOGGER << "failed " << hash;
		send_REQUEST_SLOT_FAILED();
		return true;
	}

	//find available slot
	unsigned char slot_num = 0;
	for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Incoming_Slot.begin(), iter_end = Incoming_Slot.end();
		iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->first != slot_num){
			break;
		}
		++slot_num;
	}

	//create SLOT_ID response message
	if(M = slot_iter->create_REQUEST_SLOT(slot_num)){
		LOGGER << "opened " << hash;
		Incoming_Slot.insert(std::make_pair(slot_num, slot_iter.get()));
		Exchange.schedule_send(M);
	}else{
		LOGGER << "failed " << hash;
		send_REQUEST_SLOT_FAILED();
	}
	return true;
}

bool slot_manager::recv_REQUEST_SLOT_FAILED(boost::shared_ptr<exchange::message> & M)
{

LOGGER << "stub"; exit(1);
}

void slot_manager::resume(const std::string & peer_ID)
{
	std::set<std::string> hash = database::table::join::resume_hash(peer_ID);
	for(std::set<std::string>::iterator iter_cur = hash.begin(), iter_end = hash.end();
		iter_cur != iter_end; ++iter_cur)
	{
		share::slot_iterator slot_iter = share::singleton().find_slot(*iter_cur);
		if(slot_iter != share::singleton().end_slot()){
			Pending_Slot_Request.push_back(slot_iter.get());
		}
	}
	make_slot_requests();
}

void slot_manager::send_CLOSE_SLOT(const unsigned char slot_ID)
{
	boost::shared_ptr<exchange::message> M(new exchange::message());
	M->send_buf.append(protocol::CLOSE_SLOT).append(slot_ID);
	Exchange.schedule_send(M);
}

void slot_manager::send_REQUEST_SLOT_FAILED()
{
	boost::shared_ptr<exchange::message> M(new exchange::message());
	M->send_buf.append(protocol::REQUEST_SLOT_FAILED);
	Exchange.schedule_send(M);
}
