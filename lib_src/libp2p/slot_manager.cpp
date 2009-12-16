#include "slot_manager.hpp"

slot_manager::slot_manager(
	boost::function<void (boost::shared_ptr<message>)> send_in
):
	send(send_in),
	open_slots(0)
{

}

void slot_manager::make_slot_requests(network::connection_info & CI)
{
	while(!Pending_Slot_Request.empty() && open_slots < 256){
/*
		boost::shared_ptr<message> M(new slot_request
		M->buf
			.append(protocol::REQUEST_SLOT)
			.append(convert::hex_to_bin(Pending_Slot_Request.front()->hash()));
		send(M);
		Pending_Slot_Request.pop_front();
*/
	}
}

bool slot_manager::recv_request_slot(network::connection_info & CI)
{
/*
	if(Incoming_Slot.size() > 256){
		LOGGER << CI.IP << " requested too many slots";
		database::table::blacklist::add(CI.IP);
		return false;
	}

	if(CI.recv_buf.size() < protocol::REQUEST_SLOT_SIZE){
		return false;
	}

	//unmarshal data and erase incoming message from recv_buf
	std::string hash = convert::bin_to_hex(std::string(
		reinterpret_cast<const char *>(CI.recv_buf.data()+1), SHA1::bin_size));
	CI.recv_buf.erase(0, protocol::REQUEST_SLOT_SIZE);

	//locate requested slot
	share::slot_iterator slot_iter = share::singleton().find_slot(hash);
	if(slot_iter == share::singleton().end_slot()){
		LOGGER << "failed " << hash;
		network::buffer send_buf;
		send_buf.append(protocol::ERROR);
		Send.push_back(send_buf);
		return true;
	}

	//find available slot number
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

	//determine status byte
	unsigned char status;
	if(slot_iter->Hash_Tree.complete() && slot_iter->File.complete()){
		status = 0;
	}else if(slot_iter->Hash_Tree.complete() && !slot_iter->File.complete()){
		status = 1;
	}else if(!slot_iter->Hash_Tree.complete() && slot_iter->File.complete()){
		status = 2;
	}else if(!slot_iter->Hash_Tree.complete() && !slot_iter->File.complete()){
		status = 3;
	}

	//prepare SLOT message
	network::buffer send_buf;
	send_buf
		.append(protocol::SLOT)
		.append(slot_num)
		.append(status)
		.append(convert::encode(slot_iter->file_size()));
	send_buf.tail_reserve(SHA1::bin_size);
	if(!database::pool::get()->blob_read(slot_iter->Hash_Tree.blob,
		reinterpret_cast<char *>(send_buf.tail_start()), SHA1::bin_size, 0))
	{
		LOGGER << "failed " << hash;
		network::buffer send_buf;
		send_buf.append(protocol::ERROR);
		Send.push_back(send_buf);
		return true;
	}
	send_buf.tail_resize(SHA1::bin_size);
	SHA1 SHA;
	SHA.init();
	SHA.load(reinterpret_cast<const char *>(send_buf.data()) + 3,
		8 + SHA1::bin_size);
	SHA.end();
	if(SHA.hex() != slot_iter->hash()){
		LOGGER << "failed " << hash;
		network::buffer send_buf;
		send_buf.append(protocol::ERROR);
		Send.push_back(send_buf);
		return true;
	}

	//send SLOT message
	Send.push_back(send_buf);
*/
}

bool slot_manager::recv_request_slot_failed(network::connection_info & CI)
{
/*
	LOGGER << "failed to open slot";
	CI.recv_buf.erase(0, protocol::ERROR_SIZE);
	Slot_Request.pop_front();
	return true;
*/
}

bool slot_manager::recv_slot(network::connection_info & CI)
{
/*
	if(Slot_Request.empty()){
		LOGGER << "invalid SLOT";
		database::table::blacklist::add(CI.IP);
		return false;
	}

	if(CI.recv_buf.size() < protocol::SLOT_SIZE()){
		return false;
	}

	unsigned char slot_num = CI.recv_buf[1];
	SHA1 SHA;
	SHA.init();
	SHA.load(reinterpret_cast<const char *>(CI.recv_buf.data()) + 3,
		8 + SHA1::bin_size);
	SHA.end();
	CI.recv_buf.erase(0, protocol::SLOT_SIZE());
	if(SHA.hex() != Slot_Request.front()->hash()){
		LOGGER << "invalid SLOT";
		database::table::blacklist::add(CI.IP);
		return true;
	}
	std::pair<std::map<unsigned char, boost::shared_ptr<slot> >::iterator, bool>
		ret = Outgoing_Slot.insert(std::make_pair(slot_num, Slot_Request.front()));
	Slot_Request.pop_front();
	if(ret.second == false){
		LOGGER << "invalid SLOT";
		database::table::blacklist::add(CI.IP);
		return true;
	}
	LOGGER << "received SLOT";
	return true;
*/
}

void slot_manager::resume(network::connection_info & CI, const std::string & peer_ID)
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
	make_slot_requests(CI);
}
