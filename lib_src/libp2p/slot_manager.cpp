#include "slot_manager.hpp"

slot_manager::slot_manager(exchange & Exchange_in):
	Exchange(Exchange_in)
{

}

bool slot_manager::is_slot_message(boost::shared_ptr<exchange::message> & M)
{
	assert(M && !M->recv_buf.empty());

	//check if a response to a slot message
	if(!M->send_buf.empty() && (
		M->send_buf[0] == protocol::REQUEST_SLOT ||
		M->send_buf[0] == protocol::REQUEST_HASH_TREE_BLOCK ||
		M->send_buf[0] == protocol::REQUEST_FILE_BLOCK))
	{
		return true;
	}

	//check if slot message
	if(M->recv_buf[0] == protocol::REQUEST_SLOT ||
		M->recv_buf[0] == protocol::SLOT ||
		M->recv_buf[0] == protocol::HAVE_HASH_TREE_BLOCK ||
		M->recv_buf[0] == protocol::HAVE_FILE_BLOCK ||
		M->recv_buf[0] == protocol::CLOSE_SLOT
	){
		return true;
	}

	//not a slot message
	return false;
}

bool slot_manager::recv(boost::shared_ptr<exchange::message> & M)
{
	assert(M && !M->recv_buf.empty());
	if(M->recv_buf[0] == protocol::REQUEST_SLOT){
		return recv_request_slot(M);
	}else if(!M->send_buf.empty() && M->send_buf[0] == protocol::REQUEST_SLOT
		&& M->recv_buf[0] == protocol::ERROR)
	{
		return recv_request_slot_failed(M);
	}else if(M->recv_buf[0] == protocol::SLOT){
		return recv_slot(M);
	}
	LOGGER << "unhandled message"; exit(1);
}

void slot_manager::make_slot_requests()
{
	while(!Pending_Slot_Request.empty() &&
		Slot_Request.size() + Outgoing_Slot.size() < 256)
	{
		boost::shared_ptr<exchange::message> M(new exchange::message());
		M->send_buf.append(protocol::REQUEST_SLOT)
			.append(convert::hex_to_bin(Pending_Slot_Request.front()->hash()));
		M->expected_response.push_back(std::make_pair(protocol::SLOT,
			protocol::SLOT_SIZE()));
		M->expected_response.push_back(std::make_pair(protocol::ERROR,
			protocol::ERROR_SIZE));
		/*
		The possible SLOT messages with bit fields appended to them will be
		handled by exchange.
		*/
		Exchange.schedule_send(M);
		Slot_Request.push_back(Pending_Slot_Request.front());
		Pending_Slot_Request.pop_front();
	}
}

bool slot_manager::recv_request_slot(boost::shared_ptr<exchange::message> & M)
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
		boost::shared_ptr<exchange::message> M(new exchange::message());
		M->send_buf.append(protocol::ERROR);
		Exchange.schedule_send(M);
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

	//create SLOT response message
	if(M = slot_iter->create_request_slot(slot_num)){
		LOGGER << "opened " << hash;
		Incoming_Slot.insert(std::make_pair(slot_num, slot_iter.get()));
		Exchange.schedule_send(M);
	}else{
		LOGGER << "failed " << hash;
		boost::shared_ptr<exchange::message> M(new exchange::message());
		M->send_buf.append(protocol::ERROR);
		Exchange.schedule_send(M);
	}
	return true;
}

bool slot_manager::recv_request_slot_failed(boost::shared_ptr<exchange::message> & M)
{
	LOGGER << "open failed "
		<< convert::bin_to_hex(reinterpret_cast<char *>(M->send_buf.data()+1), SHA1::bin_size);
}

bool slot_manager::recv_slot(boost::shared_ptr<exchange::message> & M)
{
	if(Slot_Request.empty()){
		LOGGER << "unexpected SLOT_ID message";
		return false;
	}

	//verify validity of file size and root hash
	SHA1 SHA;
	SHA.init();
	SHA.load(reinterpret_cast<const char *>(M->recv_buf.data()) + 3,
		8 + SHA1::bin_size);
	SHA.end();
	if(SHA.hex() != Slot_Request.front()->hash()){
		LOGGER << "invalid SLOT_ID message";
		return false;
	}

	std::pair<std::map<unsigned char, boost::shared_ptr<slot> >::iterator, bool>
		ret = Outgoing_Slot.insert(std::make_pair(M->recv_buf[1], Slot_Request.front()));
	Slot_Request.pop_front();
	if(ret.second == false){
		LOGGER << "invalid SLOT_ID message";
		return false;
	}
	LOGGER << "opened "
		<< convert::bin_to_hex(reinterpret_cast<char *>(M->send_buf.data()+1), SHA1::bin_size);
	return true;
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
