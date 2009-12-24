#include "slot_manager.hpp"

slot_manager::slot_manager(
	const int connection_ID_in,
	boost::function<void (boost::shared_ptr<message::base>)> expect_in,
	boost::function<void (boost::shared_ptr<message::base>)> send_in
):
	connection_ID(connection_ID_in),
	expect(expect_in),
	send(send_in),
	open_slots(0)
{

}

void slot_manager::make_block_requests(boost::shared_ptr<slot> S)
{
LOGGER << "woot";
/*
	if(!S->Hash_Tree.complete()){
		int unfullfilled = S->Hash_Tree.

	}else if(!S->File.complete()){

	}
*/
}

void slot_manager::make_slot_requests()
{
	while(!Pending.empty() && open_slots < 256){
		share::slot_iterator slot_iter = share::singleton().find_slot(Pending.front());
		Pending.pop();
		if(slot_iter == share::singleton().end_slot()){
			continue;
		}
		++open_slots;
		boost::shared_ptr<message::base> M_request(new message::request_slot(
			slot_iter->hash()));
		send(M_request);
		boost::shared_ptr<message::composite> M_response(new message::composite());
		M_response->add(boost::shared_ptr<message::base>(new message::slot(
			boost::bind(&slot_manager::recv_slot, this, _1, slot_iter->hash()),
			slot_iter->hash())));
		M_response->add(boost::shared_ptr<message::base>(new message::error(
			boost::bind(&slot_manager::recv_request_slot_failed, this, _1))));
		expect(M_response);
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
LOGGER << "stub: add support for incomplete"; exit(1);
	}else if(!slot_iter->Hash_Tree.complete() && slot_iter->File.complete()){
		status = 2;
LOGGER << "stub: add support for incomplete"; exit(1);
	}else if(!slot_iter->Hash_Tree.complete() && !slot_iter->File.complete()){
		status = 3;
LOGGER << "stub: add support for incomplete"; exit(1);
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
//DEBUG, converting root hash to bin -> hex, then hex -> bin, this is inefficient
	boost::shared_ptr<message::base> M_send(new message::slot(slot_num, status,
		slot_iter->file_size(), convert::bin_to_hex(buf+8, SHA1::bin_size)));
	send(M_send);

	//add slot
	std::pair<std::map<unsigned char, boost::shared_ptr<slot> >::iterator, bool>
		ret = Incoming_Slot.insert(std::make_pair(slot_num, slot_iter.get()));
	assert(ret.second);
	return true;
}

bool slot_manager::recv_request_slot_failed(boost::shared_ptr<message::base> M)
{
//DEBUG, eventually handle this failure by removing source
LOGGER;
	--open_slots;
	return true;
}


bool slot_manager::recv_slot(boost::shared_ptr<message::base> M,
	const std::string hash)
{
	share::slot_iterator slot_iter = share::singleton().find_slot(hash);
	if(slot_iter == share::singleton().end_slot()){
		LOGGER << "failed " << hash;
		boost::shared_ptr<message::base> M(new message::error());
		send(M);
		return true;
	}
	std::pair<std::map<unsigned char, boost::shared_ptr<slot> >::iterator, bool>
		ret = Outgoing_Slot.insert(std::make_pair(M->buf[1], slot_iter.get()));
	if(ret.second == false){
		//host sent duplicate slot
		LOGGER << "violated protocol";
		return false;
	}
	LOGGER << hash;
	if(M->buf[2] == 0){
/* DEBUG
It's possible we will get the file size at this point. We need a function within
slot to take the file size. The postcondition of this function will be that the
hash tree and file will be instantiated.
*/
		//slot_iter->Hash_Tree.Block_Request.
	}else if(M->buf[2] == 1){
LOGGER << "stub: add support for incomplete"; exit(1);
	}else if(M->buf[2] == 2){
LOGGER << "stub: add support for incomplete"; exit(1);
	}else if(M->buf[2] == 3){
LOGGER << "stub: add support for incomplete"; exit(1);
	}

	make_block_requests(slot_iter.get());
	return true;
}

void slot_manager::resume(const std::string & peer_ID)
{
	std::set<std::string> hash = database::table::join::resume_hash(peer_ID);
	for(std::set<std::string>::iterator iter_cur = hash.begin(), iter_end = hash.end();
		iter_cur != iter_end; ++iter_cur)
	{
		Pending.push(*iter_cur);
	}
	make_slot_requests();
}
