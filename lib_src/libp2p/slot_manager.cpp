#include "slot_manager.hpp"

slot_manager::slot_manager(
	boost::function<void (boost::shared_ptr<message::base>)> expect_in,
	boost::function<void (boost::shared_ptr<message::base>)> send_in
):
	expect(expect_in),
	send(send_in),
	open_slots(0)
{

}

void slot_manager::make_slot_requests()
{
	while(!Pending_Slot_Request.empty() && open_slots < 256){
		++open_slots;
		boost::shared_ptr<message::base> M_request(new message::request_slot(
			Pending_Slot_Request.front()->hash()));
		send(M_request);
		boost::shared_ptr<message::composite> M_response(new message::composite());
		M_response->add(boost::shared_ptr<message::base>(new message::slot(
			boost::bind(&slot_manager::recv_slot, this, _1,
			Pending_Slot_Request.front()->hash()),
			Pending_Slot_Request.front()->hash())));
		M_response->add(boost::shared_ptr<message::base>(new message::error(
			boost::bind(&slot_manager::recv_request_slot_failed, this, _1))));
		expect(M_response);
		Pending_Slot_Request.pop_front();
	}
}

bool slot_manager::recv_request_slot(boost::shared_ptr<message::base> M)
{
	//requested hash
	std::string hash = convert::bin_to_hex(
		reinterpret_cast<const char *>(M->buf.data()+1), SHA1::bin_size);
	LOGGER << hash;

	//locate requested slot
	share::slot_iterator slot_iter = share::singleton().find_slot(hash);
	if(slot_iter == share::singleton().end_slot()){
		LOGGER << "failed " << hash;
		boost::shared_ptr<message::base> M(new message::error());
		send(M);
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
LOGGER << "stub: add support for incomplete";
	}else if(!slot_iter->Hash_Tree.complete() && slot_iter->File.complete()){
		status = 2;
LOGGER << "stub: add support for incomplete";
	}else if(!slot_iter->Hash_Tree.complete() && !slot_iter->File.complete()){
		status = 3;
LOGGER << "stub: add support for incomplete";
	}

	//check to make sure we have valid file_size/root_hash
	char buf[8 + SHA1::bin_size];
	std::memcpy(buf, convert::encode(slot_iter->file_size()).data(), 8);
	if(!database::pool::get()->blob_read(slot_iter->Hash_Tree.blob, buf+8,
		SHA1::bin_size, 0))
	{
		LOGGER << "failed " << hash;
		boost::shared_ptr<message::base> M(new message::error());
		send(M);
		return true;
	}
	SHA1 SHA;
	SHA.init();
	SHA.load(buf, 8 + SHA1::bin_size);
	SHA.end();
	if(SHA.hex() != slot_iter->hash()){
		LOGGER << "failed " << hash;
		boost::shared_ptr<message::base> M(new message::error());
		send(M);
		return true;
	}

	//we have all information to send slot message
//DEBUG, converting root hash to hex and back, this is inefficient
	boost::shared_ptr<message::base> M_send(new message::slot(slot_num, status,
		slot_iter->file_size(), convert::bin_to_hex(buf+8, SHA1::bin_size)));
	send(M_send);
	return true;
}

bool slot_manager::recv_request_slot_failed(boost::shared_ptr<message::base> M)
{
LOGGER;
	return true;
}


bool slot_manager::recv_slot(boost::shared_ptr<message::base> M,
	const std::string hash)
{
LOGGER << hash;
/*
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
