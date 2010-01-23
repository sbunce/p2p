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
			iter_cur->second->unregister_download();
			Exchange.send(boost::shared_ptr<message::base>(
				new message::close_slot(iter_cur->first)));
			Outgoing_Slot.erase(iter_cur++);
			share::singleton().garbage_collect();
		}else{
			++iter_cur;
		}
	}
}

bool slot_manager::empty()
{
	return open_slots == 0 && Incoming_Slot.empty() && Pending.empty();
}

void slot_manager::exchange_call_back()
{
	close_complete();
	send_block_requests();
	send_have();
	send_slot_requests();

	for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Incoming_Slot.begin(), iter_end = Incoming_Slot.end();
		iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->second->get_transfer()
			&& iter_cur->second->get_transfer()->need_tick())
		{
			Exchange.trigger_all();
			break;
		}
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
//DEBUG, large copy
		M->buf.erase(0, 1);
		transfer::status status = iter->second->get_transfer()->write_file_block(
			Exchange.connection_ID, block_num, M->buf);
		if(status == transfer::good){
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

bool slot_manager::recv_have_file_block(boost::shared_ptr<message::base> M)
{
	boost::uint64_t block_num = convert::decode_VLI(std::string(
		reinterpret_cast<char *>(M->buf.data()) + 2, M->buf.size() - 2));
	LOGGER << block_num;
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Outgoing_Slot.find(M->buf[1]);
	if(iter != Outgoing_Slot.end()){
		if(iter->second->get_transfer()){
			iter->second->get_transfer()->recv_have_file_block(Exchange.connection_ID,
				block_num);
		}
	}
	return true;
}

bool slot_manager::recv_have_hash_tree_block(boost::shared_ptr<message::base> M)
{
	boost::uint64_t block_num = convert::decode_VLI(std::string(
		reinterpret_cast<char *>(M->buf.data()) + 2, M->buf.size() - 2));
	LOGGER << block_num;
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Outgoing_Slot.find(M->buf[1]);
	if(iter != Outgoing_Slot.end()){
		if(iter->second->get_transfer()){
			iter->second->get_transfer()->recv_have_hash_tree_block(Exchange.connection_ID,
				block_num);
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
//DEBUG, large copy
		M->buf.erase(0, 1);
		transfer::status status = iter->second->get_transfer()->write_tree_block(
			Exchange.connection_ID, block_num, M->buf);
		if(status == transfer::good){
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
		Exchange.send(boost::shared_ptr<message::base>(new message::error()));
	}else{
		assert(iter->second->get_transfer());
		boost::uint64_t block_num = convert::decode_VLI(M->buf.str(2));
		LOGGER << block_num;
		boost::shared_ptr<message::base> M_request;
		transfer::status status = iter->second->get_transfer()->read_tree_block(
			M_request, block_num);
		if(status == transfer::good){
			Exchange.send(M_request);
			return true;
		}else if(status == transfer::bad){
			LOGGER << "error reading tree block";
			Incoming_Slot.erase(iter);
			Exchange.send(boost::shared_ptr<message::base>(new message::error()));
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
		Exchange.send(boost::shared_ptr<message::base>(new message::error()));
	}else{
		assert(iter->second->get_transfer());
		boost::uint64_t block_num = convert::decode_VLI(M->buf.str(2));
		LOGGER << block_num;
		boost::shared_ptr<message::base> M_request;
		transfer::status status = iter->second->get_transfer()->read_file_block(
			M_request, block_num);
		if(status == transfer::good){
			Exchange.send(M_request);
			return true;
		}else if(status == transfer::bad){
			LOGGER << "error reading file block";
			Incoming_Slot.erase(iter);
			Exchange.send(boost::shared_ptr<message::base>(new message::error()));
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
		Exchange.send(boost::shared_ptr<message::base>(new message::error()));
		return true;
	}

	if(!slot_iter->get_transfer()){
		//we do not yet know file size
		LOGGER << "failed " << hash;
		Exchange.send(boost::shared_ptr<message::base>(new message::error()));
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

	//possible bit_fields for incomplete tree and/or file
	bit_field tree_BF, file_BF;
	slot_iter->get_transfer()->get_bit_fields(Exchange.connection_ID, tree_BF, file_BF);

	//file size
	boost::uint64_t file_size = slot_iter->file_size();
	if(file_size == 0){
		LOGGER << "failed " << hash;
		Exchange.send(boost::shared_ptr<message::base>(new message::error()));
		return true;
	}

	//root hash
	std::string root_hash;
	if(!slot_iter->get_transfer()->root_hash(root_hash)){
		LOGGER << "failed " << hash;
		Exchange.send(boost::shared_ptr<message::base>(new message::error()));
		return true;
	}

	//we have all information to send slot message
	Exchange.send(boost::shared_ptr<message::base>(new message::slot(slot_num,
		file_size, root_hash, tree_BF, file_BF)));

	//add slot
	std::pair<std::map<unsigned char, boost::shared_ptr<slot> >::iterator, bool>
		ret = Incoming_Slot.insert(std::make_pair(slot_num, slot_iter.get()));
	assert(ret.second);

	//unexpect any previous
	network::buffer buf;
	buf.append(protocol::request_hash_tree_block).append(slot_num);
	Exchange.expect_anytime_erase(buf);
	buf.clear();
	buf.append(protocol::request_file_block).append(slot_num);
	Exchange.expect_anytime_erase(buf);

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
		Exchange.send(boost::shared_ptr<message::base>(new message::close_slot(M->buf[1])));
		return true;
	}

	//file size and root hash might not be known, set them
	boost::uint64_t file_size = convert::decode<boost::uint64_t>(
		std::string(reinterpret_cast<char *>(M->buf.data()+3), 8));
	std::string root_hash = convert::bin_to_hex(
		reinterpret_cast<char *>(M->buf.data()+11), SHA1::bin_size);
	if(!slot_iter->set_unknown(Exchange.connection_ID, file_size, root_hash)){
		LOGGER << "error setting file size and root hash";
		Exchange.send(boost::shared_ptr<message::base>(new message::close_slot(M->buf[1])));
		return true;
	}

	//read bit field(s) (if any exist)
	bit_field tree_BF, file_BF;
	if(M->buf[2] == 1){
		//file bit_field
		boost::uint64_t file_block_count = file::calc_file_block_count(file_size);
		boost::uint64_t file_BF_size = bit_field::size_bytes(file_block_count, 1);
		assert(M->buf.size() == 31 + file_BF_size);
		file_BF.set_buf(M->buf.data() + 31, file_BF_size, file_block_count, 1);

		//unexpect previous have_file_block message with this slot (if exists)
		network::buffer buf;
		buf.append(protocol::have_file_block).append(M->buf[1]);
		Exchange.expect_anytime_erase(buf);

		//expect have_hash_tree_block message
		Exchange.expect_anytime(boost::shared_ptr<message::base>(new
			message::have_file_block(boost::bind(&slot_manager::recv_have_file_block,
			this, _1), M->buf[1], file_block_count)));
	}else if(M->buf[2] == 2){
		//tree bit_field
		boost::uint64_t tree_block_count = hash_tree::calc_tree_block_count(file_size);
		boost::uint64_t tree_BF_size = bit_field::size_bytes(tree_block_count, 1);
		assert(M->buf.size() == 31 + tree_BF_size);
		tree_BF.set_buf(M->buf.data() + 31, tree_BF_size, tree_block_count, 1);

		//unexpect previous have_hash_tree_block message with this slot (if exists)
		network::buffer buf;
		buf.append(protocol::have_hash_tree_block).append(M->buf[1]);
		Exchange.expect_anytime_erase(buf);

		//expect have_hash_tree_block message
		Exchange.expect_anytime(boost::shared_ptr<message::base>(new
			message::have_hash_tree_block(boost::bind(&slot_manager::recv_have_hash_tree_block,
			this, _1), M->buf[1], tree_block_count)));
	}else if(M->buf[2] == 3){
		//tree and file bit_field
		boost::uint64_t tree_block_count = hash_tree::calc_tree_block_count(file_size);
		boost::uint64_t tree_BF_size = bit_field::size_bytes(tree_block_count, 1);
		boost::uint64_t file_block_count = file::calc_file_block_count(file_size);
		boost::uint64_t file_BF_size = bit_field::size_bytes(file_block_count, 1);
		assert(M->buf.size() == 31 + tree_BF_size + file_BF_size);
		tree_BF.set_buf(M->buf.data() + 31, tree_BF_size, tree_block_count, 1);
		file_BF.set_buf(M->buf.data() + 31 + tree_BF_size, file_BF_size, file_block_count, 1);

		//unexpect previous have_hash_tree_block message with this slot (if exists)
		network::buffer buf;
		buf.append(protocol::have_hash_tree_block).append(M->buf[1]);
		Exchange.expect_anytime_erase(buf);

		//expect have_hash_tree_block message
		Exchange.expect_anytime(boost::shared_ptr<message::base>(new
			message::have_hash_tree_block(boost::bind(&slot_manager::recv_have_hash_tree_block,
			this, _1), M->buf[1], tree_block_count)));

		//unexpect previous have_file_block message with this slot (if exists)
		buf.clear();
		buf.append(protocol::have_file_block).append(M->buf[1]);
		Exchange.expect_anytime_erase(buf);

		//expect have_hash_tree_block message
		Exchange.expect_anytime(boost::shared_ptr<message::base>(new
			message::have_file_block(boost::bind(&slot_manager::recv_have_file_block,
			this, _1), M->buf[1], file_block_count)));
	}
	slot_iter->get_transfer()->register_outgoing(Exchange.connection_ID, tree_BF, file_BF);

	//add outgoing slot
	std::pair<std::map<unsigned char, boost::shared_ptr<slot> >::iterator, bool>
		ret = Outgoing_Slot.insert(std::make_pair(M->buf[1], slot_iter.get()));
	if(ret.second == false){
		//host sent duplicate slot
		LOGGER << "violated protocol";
		return false;
	}
	slot_iter->register_download();
	return true;
}

void slot_manager::remove(const std::string & hash)
{
	for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Incoming_Slot.begin(); iter_cur != Incoming_Slot.end();)
	{
		if(iter_cur->second->hash() == hash){
			/*
			The next request made for this slot will result in an error and the
			other side will close it's slot.
			*/
			iter_cur->second->unregister_upload();
			Incoming_Slot.erase(iter_cur++);
		}else{
			++iter_cur;
		}
	}
	for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Outgoing_Slot.begin(); iter_cur != Outgoing_Slot.end();)
	{
		if(iter_cur->second->hash() == hash){
			iter_cur->second->unregister_download();
			Exchange.send(boost::shared_ptr<message::base>(new message::close_slot(iter_cur->first)));
			--open_slots;
			Outgoing_Slot.erase(iter_cur++);
		}else{
			++iter_cur;
		}
	}
	for(std::list<std::string>::iterator iter_cur = Pending.begin();
		iter_cur != Pending.end();)
	{
		if(*iter_cur == hash){
			iter_cur = Pending.erase(iter_cur);
		}else{
			++iter_cur;
		}
	}
}

void slot_manager::resume(const std::string & peer_ID)
{
	std::set<std::string> hash = database::table::join::resume_hash(peer_ID);
	for(std::set<std::string>::iterator iter_cur = hash.begin(), iter_end = hash.end();
		iter_cur != iter_end; ++iter_cur)
	{
		Pending.push_back(*iter_cur);
	}
}

void slot_manager::send_block_requests()
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
			boost::uint64_t block_num;
			unsigned block_size;
			if(iter_cur->second->get_transfer()->next_request_tree(
				Exchange.connection_ID, block_num, block_size))
			{
				++pipeline_size;
				serviced_one = true;
				latest_slot = iter_cur->first;
				Exchange.send(boost::shared_ptr<message::base>(new message::request_hash_tree_block(
					iter_cur->first, block_num, iter_cur->second->get_transfer()->tree_block_count())));
				boost::shared_ptr<message::composite> M_composite(new message::composite());
				M_composite->add(boost::shared_ptr<message::block>(new message::block(boost::bind(
					&slot_manager::recv_hash_tree_block, this, _1, iter_cur->first, block_num),
					block_size, iter_cur->second->get_transfer()->download_speed_calculator())));
				M_composite->add(boost::shared_ptr<message::error>(new message::error(
					boost::bind(&slot_manager::recv_request_block_failed, this, _1, iter_cur->first))));
				Exchange.expect_response(M_composite);
			}else if(iter_cur->second->get_transfer()->next_request_file(
				Exchange.connection_ID, block_num, block_size))
			{
				++pipeline_size;
				serviced_one = true;
				latest_slot = iter_cur->first;
				Exchange.send(boost::shared_ptr<message::base>(new message::request_file_block(
					iter_cur->first, block_num, iter_cur->second->get_transfer()->file_block_count())));
				boost::shared_ptr<message::composite> M_composite(new message::composite());
				M_composite->add(boost::shared_ptr<message::block>(new message::block(boost::bind(
					&slot_manager::recv_file_block, this, _1, iter_cur->first, block_num),
					block_size, iter_cur->second->get_transfer()->download_speed_calculator())));
				M_composite->add(boost::shared_ptr<message::error>(new message::error(
					boost::bind(&slot_manager::recv_request_block_failed, this, _1, iter_cur->first))));
				Exchange.expect_response(M_composite);
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

void slot_manager::send_have()
{
	for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Incoming_Slot.begin(), iter_end = Incoming_Slot.end();
		iter_cur != iter_end; ++iter_cur)
	{
		while(true){
			boost::uint64_t block_num;
			if(iter_cur->second->get_transfer()->next_have_tree(Exchange.connection_ID,
				block_num))
			{
				LOGGER << "tree: " << block_num;
				Exchange.send(boost::shared_ptr<message::base>(
					new message::have_hash_tree_block(iter_cur->first, block_num,
					iter_cur->second->get_transfer()->tree_block_count())));
			}else if(iter_cur->second->get_transfer()->next_have_file(
				Exchange.connection_ID, block_num))
			{
				LOGGER << "file: " << block_num;
				Exchange.send(boost::shared_ptr<message::base>(
					new message::have_file_block(iter_cur->first, block_num,
					iter_cur->second->get_transfer()->file_block_count())));
			}else{
				break;
			}
		}
	}
}

void slot_manager::send_slot_requests()
{
	while(!Pending.empty() && open_slots < 256){
		share::slot_iterator slot_iter = share::singleton().find_slot(Pending.front());
		Pending.pop_front();
		if(slot_iter == share::singleton().end_slot()){
			continue;
		}
		++open_slots;
		Exchange.send(boost::shared_ptr<message::base>(new message::request_slot(
			slot_iter->hash())));
		boost::shared_ptr<message::composite> M_composite(new message::composite());
		M_composite->add(boost::shared_ptr<message::base>(new message::slot(
			boost::bind(&slot_manager::recv_slot, this, _1, slot_iter->hash()),
			slot_iter->hash())));
		M_composite->add(boost::shared_ptr<message::base>(new message::error(
			boost::bind(&slot_manager::recv_request_slot_failed, this, _1))));
		Exchange.expect_response(M_composite);
	}
}
