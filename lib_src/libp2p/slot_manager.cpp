#include "slot_manager.hpp"

slot_manager::slot_manager(
	exchange & Exchange_in
):
	Exchange(Exchange_in),
	pipeline_size(0),
	open_slots(0),
	latest_slot(0)
{
	//register possible incoming messages
	Exchange.expect_anytime(boost::shared_ptr<message::base>(new message::request_slot(
		boost::bind(&slot_manager::recv_request_slot, this, _1))));
	Exchange.expect_anytime(boost::shared_ptr<message::base>(new message::close_slot(
		boost::bind(&slot_manager::recv_close_slot, this, _1))));
}

slot_manager::~slot_manager()
{
	for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Incoming_Slot.begin(), iter_end = Incoming_Slot.end();
		iter_cur != iter_end; ++iter_cur)
	{
		iter_cur->second->unregister_upload();
	}
	for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Outgoing_Slot.begin(), iter_end = Outgoing_Slot.end();
		iter_cur != iter_end; ++iter_cur)
	{
		iter_cur->second->unregister_download();
	}
	share::singleton().garbage_collect();
}

void slot_manager::close_complete()
{
	for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Outgoing_Slot.begin(); iter_cur != Outgoing_Slot.end();)
	{
		if(iter_cur->second->get_transfer()
			&& iter_cur->second->get_transfer()->complete())
		{
			Exchange.send(boost::shared_ptr<message::base>(
				new message::close_slot(iter_cur->first)));
			Outgoing_Slot.erase(iter_cur++);
			share::singleton().garbage_collect();
		}else{
			++iter_cur;
		}
	}
}

void slot_manager::make_block_requests()
{
	if(Outgoing_Slot.empty()){
		return;
	}
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Outgoing_Slot.upper_bound(latest_slot);
	if(iter_cur == Outgoing_Slot.end()){
		iter_cur = Outgoing_Slot.begin();
	}

	/*
	These are used to break out of the loop if we loop through all slots and none
	need to make a request.
	*/
	unsigned char start_slot = iter_cur->first;
	bool serviced_one = false;

	while(pipeline_size <= protocol::max_block_pipeline){
		if(iter_cur->second->get_transfer()){
			//make block requests
			boost::uint64_t block_num; //block number to request
			unsigned block_size;       //size of block
			if(iter_cur->second->get_transfer()->next_request_tree(
				Exchange.connection_ID, block_num, block_size))
			{
				++pipeline_size;
				serviced_one = true;
				latest_slot = iter_cur->first;
				boost::shared_ptr<message::base> M_request(new message::request_hash_tree_block(
					iter_cur->first, block_num, iter_cur->second->get_transfer()->tree_block_count()));
				Exchange.send(M_request);
				boost::shared_ptr<message::composite> M_response(new message::composite());
				M_response->add(boost::shared_ptr<message::block>(new message::block(boost::bind(
					&slot_manager::recv_hash_tree_block, this, _1, iter_cur->first, block_num),
					block_size)));
				M_response->add(boost::shared_ptr<message::error>(new message::error(
					boost::bind(&slot_manager::recv_request_block_failed, this, _1, iter_cur->first))));
				Exchange.expect_response(M_response);
			}else if(iter_cur->second->get_transfer()->next_request_file(
				Exchange.connection_ID, block_num, block_size))
			{
				++pipeline_size;
				serviced_one = true;
				latest_slot = iter_cur->first;
				boost::shared_ptr<message::base> M_request(new message::request_file_block(
					iter_cur->first, block_num, iter_cur->second->get_transfer()->file_block_count()));
				Exchange.send(M_request);
				boost::shared_ptr<message::composite> M_response(new message::composite());
				M_response->add(boost::shared_ptr<message::block>(new message::block(boost::bind(
					&slot_manager::recv_file_block, this, _1, iter_cur->first, block_num),
					block_size)));
				M_response->add(boost::shared_ptr<message::error>(new message::error(
					boost::bind(&slot_manager::recv_request_block_failed, this, _1, iter_cur->first))));
				Exchange.expect_response(M_response);
			}
		}
		++iter_cur;
		if(iter_cur == Outgoing_Slot.end()){
			iter_cur = Outgoing_Slot.begin();
		}
		if(iter_cur->first == start_slot && !serviced_one){
			//looped through all sockets and none needed to be serviced
			break;
		}else if(iter_cur->first == start_slot && serviced_one){
			//reset flag for next loop through slots
			serviced_one = false;
		}
	}
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
		Exchange.send(M_request);
		boost::shared_ptr<message::composite> M_response(new message::composite());
		M_response->add(boost::shared_ptr<message::base>(new message::slot(
			boost::bind(&slot_manager::recv_slot, this, _1, slot_iter->hash()),
			slot_iter->hash())));
		M_response->add(boost::shared_ptr<message::base>(new message::error(
			boost::bind(&slot_manager::recv_request_slot_failed, this, _1))));
		Exchange.expect_response(M_response);
	}
}

bool slot_manager::recv_close_slot(boost::shared_ptr<message::base> M)
{
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Incoming_Slot.find(M->buf[1]);
	if(iter != Incoming_Slot.end()){
		LOGGER;
		Incoming_Slot.erase(iter);
		return true;
	}else{
		LOGGER << "slot already closed";
		return false;
	}
}

bool slot_manager::recv_file_block(boost::shared_ptr<message::base> M,
	const unsigned slot_num, const boost::uint64_t block_num)
{
	--pipeline_size;
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Outgoing_Slot.find(slot_num);
	if(iter != Outgoing_Slot.end()){
		assert(iter->second->get_transfer());
		M->buf.erase(0, 1);
		transfer::status status = iter->second->get_transfer()->write_file_block(
			Exchange.connection_ID, block_num, M->buf);
		if(status == transfer::good){
			make_block_requests();
			close_complete();
			return true;
		}else if(status == transfer::bad){
			LOGGER << "stub: need to close slot";
			exit(1);
		}else if(status == transfer::protocol_violated){
			return false;
		}
	}
	return true;
}

bool slot_manager::recv_hash_tree_block(boost::shared_ptr<message::base> M,
	const unsigned slot_num, const boost::uint64_t block_num)
{
	--pipeline_size;
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Outgoing_Slot.find(slot_num);
	if(iter != Outgoing_Slot.end()){
		assert(iter->second->get_transfer());
		M->buf.erase(0, 1);
		transfer::status status = iter->second->get_transfer()->write_tree_block(
			Exchange.connection_ID, block_num, M->buf);
		if(status == transfer::good){
			make_block_requests();
			close_complete();
			return true;
		}else if(status == transfer::bad){
			LOGGER << "stub: need to close slot";
			exit(1);
		}else if(status == transfer::protocol_violated){
			return false;
		}
	}
	return true;
}

bool slot_manager::recv_request_block_failed(boost::shared_ptr<message::base> M,
	const unsigned slot_num)
{
	LOGGER;
	Outgoing_Slot.erase(slot_num);
	return true;
}

bool slot_manager::recv_request_hash_tree_block(
	boost::shared_ptr<message::base> M, const unsigned slot_num)
{
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Incoming_Slot.find(slot_num);
	if(iter == Incoming_Slot.end()){
		boost::shared_ptr<message::base> M(new message::error());
		Exchange.send(M);
	}else{
		assert(iter->second->get_transfer());
		boost::uint64_t block_num = convert::decode_VLI(M->buf.str(2));
		LOGGER << block_num;
		boost::shared_ptr<message::base> M;
		transfer::status status = iter->second->get_transfer()->read_tree_block(
			M, block_num);
		if(status == transfer::good){
			Exchange.send(M);
			return true;
		}else if(status == transfer::bad){
			LOGGER << "error reading tree block";
			Incoming_Slot.erase(iter);
			boost::shared_ptr<message::base> M(new message::error());
			Exchange.send(M);
			return true;
		}else if(status == transfer::protocol_violated){
			return false;
		}
	}
	return true;
}

bool slot_manager::recv_request_file_block(
	boost::shared_ptr<message::base> M, const unsigned slot_num)
{
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Incoming_Slot.find(slot_num);
	if(iter == Incoming_Slot.end()){
		boost::shared_ptr<message::base> M(new message::error());
		Exchange.send(M);
	}else{
		assert(iter->second->get_transfer());
		boost::uint64_t block_num = convert::decode_VLI(M->buf.str(2));
		LOGGER << block_num;
		boost::shared_ptr<message::base> M;
		transfer::status status = iter->second->get_transfer()->read_file_block(
			M, block_num);
		if(status == transfer::good){
			Exchange.send(M);
			return true;
		}else if(status == transfer::bad){
			LOGGER << "error reading file block";
			Incoming_Slot.erase(iter);
			boost::shared_ptr<message::base> M(new message::error());
			Exchange.send(M);
			return true;
		}else if(status == transfer::protocol_violated){
			return false;
		}
	}
	return true;
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
		Exchange.send(M);
		return true;
	}

	if(!slot_iter->get_transfer()){
		//we do not yet know file size
		LOGGER << "failed " << hash;
		boost::shared_ptr<message::base> M(new message::error());
		Exchange.send(M);
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

	//status byte (third byte of slot message)
	unsigned char status;
	if(!slot_iter->get_transfer()->get_status(status)){
		LOGGER << "failed " << hash;
		boost::shared_ptr<message::base> M(new message::error());
		Exchange.send(M);
		return true;
	}

	//file size
	boost::uint64_t file_size = slot_iter->file_size();
	if(file_size == 0){
		LOGGER << "failed " << hash;
		boost::shared_ptr<message::base> M(new message::error());
		Exchange.send(M);
		return true;
	}

	//root hash
	std::string root_hash;
	if(!slot_iter->get_transfer()->root_hash(root_hash)){
		LOGGER << "failed " << hash;
		boost::shared_ptr<message::base> M(new message::error());
		Exchange.send(M);
		return true;
	}

	//we have all information to send slot message
	boost::shared_ptr<message::base> M_send(new message::slot(slot_num, status,
		file_size, root_hash));
	Exchange.send(M_send);

	//add slot
	std::pair<std::map<unsigned char, boost::shared_ptr<slot> >::iterator, bool>
		ret = Incoming_Slot.insert(std::make_pair(slot_num, slot_iter.get()));
	assert(ret.second);

	//expect incoming block requests
	Exchange.expect_anytime(boost::shared_ptr<message::base>(
		new message::request_hash_tree_block(
		boost::bind(&slot_manager::recv_request_hash_tree_block, this, _1, slot_num),
		slot_num, slot_iter->get_transfer()->tree_block_count())));
	Exchange.expect_anytime(boost::shared_ptr<message::base>(
		new message::request_file_block(
		boost::bind(&slot_manager::recv_request_file_block, this, _1, slot_num),
		slot_num, slot_iter->get_transfer()->file_block_count())));

	slot_iter->register_upload();

	return true;
}

bool slot_manager::recv_request_slot_failed(boost::shared_ptr<message::base> M)
{
	LOGGER << "stub: handle request slot failure";
	--open_slots;
	return true;
}

bool slot_manager::recv_slot(boost::shared_ptr<message::base> M,
	const std::string hash)
{
	LOGGER << hash;
	share::slot_iterator slot_iter = share::singleton().find_slot(hash);
	if(slot_iter == share::singleton().end_slot()){
		LOGGER << "failed " << hash;
		boost::shared_ptr<message::base> M(new message::error());
		Exchange.send(M);
		return true;
	}

	//file size and root hash might not be known, set them
	boost::uint64_t file_size = convert::decode<boost::uint64_t>(
		std::string(reinterpret_cast<char *>(M->buf.data()+3), 8));
	std::string root_hash = convert::bin_to_hex(
		reinterpret_cast<char *>(M->buf.data()+11), SHA1::bin_size);
	if(!slot_iter->set_unknown(Exchange.connection_ID, file_size, root_hash)){
		LOGGER << "error setting file size and root hash";
		boost::shared_ptr<message::base> M(new message::close_slot(M->buf[1]));
		Exchange.send(M);
		return true;
	}

	//read bit field(s) (if any exist)
	if(M->buf[2] == 0){
		slot_iter->get_transfer()->register_outgoing_0(Exchange.connection_ID);
	}else if(M->buf[2] == 1){
		LOGGER << "stub: add support for incomplete";
		exit(1);
	}else if(M->buf[2] == 2){
		LOGGER << "stub: add support for incomplete";
		exit(1);
	}else if(M->buf[2] == 3){
		LOGGER << "stub: add support for incomplete";
		exit(1);
	}

	//add outgoing slot
	std::pair<std::map<unsigned char, boost::shared_ptr<slot> >::iterator, bool>
		ret = Outgoing_Slot.insert(std::make_pair(M->buf[1], slot_iter.get()));
	if(ret.second == false){
		//host sent duplicate slot
		LOGGER << "violated protocol";
		return false;
	}
	slot_iter->register_download();

	//make initial block requests
	make_block_requests();
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
