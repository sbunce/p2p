#include "slot_manager.hpp"

slot_manager::slot_manager(
	exchange_tcp & Exchange_in,
	boost::function<void(const int)> trigger_tick_in
):
	Exchange(Exchange_in),
	trigger_tick(trigger_tick_in),
	pipeline_size(0),
	open_slots(0),
	latest_slot(0)
{
	//register possible incoming messages
	Exchange.expect_anytime(boost::shared_ptr<message_tcp::base>(new message_tcp::request_slot(
		boost::bind(&slot_manager::recv_request_slot, this, _1))));
	Exchange.expect_anytime(boost::shared_ptr<message_tcp::base>(new message_tcp::close_slot(
		boost::bind(&slot_manager::recv_close_slot, this, _1))));
}

slot_manager::~slot_manager()
{
	for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Incoming_Slot.begin(), iter_end = Incoming_Slot.end();
		iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->second->get_transfer()){
			iter_cur->second->get_transfer()->incoming_unsubscribe(Exchange.connection_ID);
		}
	}
	for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Outgoing_Slot.begin(), iter_end = Outgoing_Slot.end();
		iter_cur != iter_end; ++iter_cur)
	{
		if(iter_cur->second->get_transfer()){
			iter_cur->second->get_transfer()->outgoing_unsubscribe(Exchange.connection_ID);
		}
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
			if(iter_cur->second->get_transfer()){
				iter_cur->second->get_transfer()->outgoing_unsubscribe(Exchange.connection_ID);
			}
			Exchange.send(boost::shared_ptr<message_tcp::base>(
				new message_tcp::close_slot(iter_cur->first)));
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

bool slot_manager::recv_close_slot(network::buffer & buf)
{
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Incoming_Slot.find(buf[1]);
	if(iter != Incoming_Slot.end()){
		LOGGER;
		if(iter->second->get_transfer()){
			iter->second->get_transfer()->incoming_unsubscribe(Exchange.connection_ID);
		}
		Incoming_Slot.erase(iter);
		return true;
	}else{
		LOGGER << "slot already closed";
		return false;
	}
}

bool slot_manager::recv_file_block(network::buffer & buf,
	const unsigned slot_num, const boost::uint64_t block_num)
{
	--pipeline_size;
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Outgoing_Slot.find(slot_num);
	if(iter != Outgoing_Slot.end()){
		assert(iter->second->get_transfer());
//DEBUG, large copy
		buf.erase(0, 1);
		transfer::status status = iter->second->get_transfer()->write_file_block(
			Exchange.connection_ID, block_num, buf);
		if(status == transfer::good){
			return true;
		}else if(status == transfer::bad){
			LOGGER << "error writing file, closing slot";
			if(iter->second->get_transfer()){
				iter->second->get_transfer()->outgoing_unsubscribe(Exchange.connection_ID);
			}
			Exchange.send(boost::shared_ptr<message_tcp::base>(
				new message_tcp::close_slot(iter->first)));
			Outgoing_Slot.erase(iter);
			share::singleton().garbage_collect();
		}else if(status == transfer::protocol_violated){
			return false;
		}
	}
	return true;
}

bool slot_manager::recv_have_file_block(network::buffer & buf)
{
	boost::uint64_t block_num = convert::bin_VLI_to_int(std::string(
		reinterpret_cast<char *>(buf.data()) + 2, buf.size() - 2));
	LOGGER << block_num;
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Outgoing_Slot.find(buf[1]);
	if(iter != Outgoing_Slot.end()){
		if(iter->second->get_transfer()){
			iter->second->get_transfer()->recv_have_file_block(
				Exchange.connection_ID, block_num);
		}
	}
	return true;
}

bool slot_manager::recv_have_hash_tree_block(network::buffer & buf)
{
	boost::uint64_t block_num = convert::bin_VLI_to_int(std::string(
		reinterpret_cast<char *>(buf.data()) + 2, buf.size() - 2));
	LOGGER << block_num;
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Outgoing_Slot.find(buf[1]);
	if(iter != Outgoing_Slot.end()){
		if(iter->second->get_transfer()){
			iter->second->get_transfer()->recv_have_hash_tree_block(
				Exchange.connection_ID, block_num);
		}
	}
	return true;
}

bool slot_manager::recv_hash_tree_block(network::buffer & buf,
	const unsigned slot_num, const boost::uint64_t block_num)
{
	--pipeline_size;
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Outgoing_Slot.find(slot_num);
	if(iter != Outgoing_Slot.end()){
		assert(iter->second->get_transfer());
//DEBUG, large copy
		buf.erase(0, 1);
		transfer::status status = iter->second->get_transfer()->write_tree_block(
			Exchange.connection_ID, block_num, buf);
		if(status == transfer::good){
			return true;
		}else if(status == transfer::bad){
			LOGGER << "error writing hash tree, closing slot";
			if(iter->second->get_transfer()){
				iter->second->get_transfer()->outgoing_unsubscribe(Exchange.connection_ID);
			}
			Exchange.send(boost::shared_ptr<message_tcp::base>(
				new message_tcp::close_slot(iter->first)));
			Outgoing_Slot.erase(iter);
			share::singleton().garbage_collect();
		}else if(status == transfer::protocol_violated){
			return false;
		}
	}
	return true;
}

bool slot_manager::recv_request_block_failed(network::buffer & buf,
	const unsigned slot_num)
{
	LOGGER;
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Outgoing_Slot.find(slot_num);
	if(iter != Outgoing_Slot.end()){
		if(iter->second->get_transfer()){
			iter->second->get_transfer()->outgoing_unsubscribe(Exchange.connection_ID);
		}
		Outgoing_Slot.erase(iter);
	}
	return true;
}

bool slot_manager::recv_request_hash_tree_block(
	network::buffer & buf, const unsigned slot_num)
{
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Incoming_Slot.find(slot_num);
	if(iter == Incoming_Slot.end()){
		Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::error()));
	}else{
		assert(iter->second->get_transfer());
		boost::uint64_t block_num = convert::bin_VLI_to_int(buf.str(2));
		LOGGER << block_num;
		boost::shared_ptr<message_tcp::base> M_request;
		transfer::status status = iter->second->get_transfer()->read_tree_block(
			M_request, block_num);
		if(status == transfer::good){
			Exchange.send(M_request);
			return true;
		}else if(status == transfer::bad){
			LOGGER << "error reading tree block";
			if(iter->second->get_transfer()){
				iter->second->get_transfer()->incoming_unsubscribe(Exchange.connection_ID);
			}
			Incoming_Slot.erase(iter);
			Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::error()));
			return true;
		}else if(status == transfer::protocol_violated){
			return false;
		}
	}
	return true;
}

bool slot_manager::recv_request_file_block(
	network::buffer & buf, const unsigned slot_num)
{
	std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter = Incoming_Slot.find(slot_num);
	if(iter == Incoming_Slot.end()){
		Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::error()));
	}else{
		assert(iter->second->get_transfer());
		boost::uint64_t block_num = convert::bin_VLI_to_int(buf.str(2));
		LOGGER << block_num;
		boost::shared_ptr<message_tcp::base> M_request;
		transfer::status status = iter->second->get_transfer()->read_file_block(
			M_request, block_num);
		if(status == transfer::good){
			Exchange.send(M_request);
			return true;
		}else if(status == transfer::bad){
			LOGGER << "error reading file block";
			if(iter->second->get_transfer()){
				iter->second->get_transfer()->incoming_unsubscribe(Exchange.connection_ID);
			}
			Incoming_Slot.erase(iter);
			Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::error()));
			return true;
		}else if(status == transfer::protocol_violated){
			return false;
		}
	}
	return true;
}

bool slot_manager::recv_request_slot(network::buffer & buf)
{
	//requested hash
	std::string hash = convert::bin_to_hex(std::string(
		reinterpret_cast<const char *>(buf.data()+1), SHA1::bin_size));
	LOGGER << hash;

	//locate requested slot
	share::slot_iterator slot_iter = share::singleton().find_slot(hash);
	if(slot_iter == share::singleton().end_slot()){
		LOGGER << "failed " << hash;
		Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::error()));
		return true;
	}

	if(!slot_iter->get_transfer()){
		//we do not yet know file size
		LOGGER << "failed " << hash;
		Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::error()));
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
	slot_iter->get_transfer()->incoming_subscribe(Exchange.connection_ID,
		trigger_tick, tree_BF, file_BF);

	//file size
	boost::uint64_t file_size = slot_iter->file_size();
	if(file_size == 0){
		LOGGER << "failed " << hash;
		Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::error()));
		return true;
	}

	//root hash
	std::string root_hash;
	if(!slot_iter->get_transfer()->root_hash(root_hash)){
		LOGGER << "failed " << hash;
		Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::error()));
		return true;
	}

	//we have all information to send slot message
	Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::slot(slot_num,
		file_size, root_hash, tree_BF, file_BF)));

	//add slot
	std::pair<std::map<unsigned char, boost::shared_ptr<slot> >::iterator, bool>
		ret = Incoming_Slot.insert(std::make_pair(slot_num, slot_iter.get()));
	assert(ret.second);

	//unexpect any previous
	network::buffer unexpect_buf;
	unexpect_buf.append(protocol::request_hash_tree_block).append(slot_num);
	Exchange.expect_anytime_erase(unexpect_buf);
	unexpect_buf.clear();
	unexpect_buf.append(protocol::request_file_block).append(slot_num);
	Exchange.expect_anytime_erase(unexpect_buf);

	//expect incoming block requests
	Exchange.expect_anytime(boost::shared_ptr<message_tcp::base>(
		new message_tcp::request_hash_tree_block(
		boost::bind(&slot_manager::recv_request_hash_tree_block, this, _1, slot_num),
		slot_num, slot_iter->get_transfer()->tree_block_count())));
	Exchange.expect_anytime(boost::shared_ptr<message_tcp::base>(
		new message_tcp::request_file_block(
		boost::bind(&slot_manager::recv_request_file_block, this, _1, slot_num),
		slot_num, slot_iter->get_transfer()->file_block_count())));

	return true;
}

bool slot_manager::recv_request_slot_failed(network::buffer & buf)
{
	LOGGER << "stub: handle request slot failure";
	--open_slots;
	return true;
}

bool slot_manager::recv_slot(network::buffer & buf,
	const std::string hash)
{
	LOGGER << hash;
	share::slot_iterator slot_iter = share::singleton().find_slot(hash);
	if(slot_iter == share::singleton().end_slot()){
		LOGGER << "failed " << hash;
		Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::close_slot(buf[1])));
		return true;
	}

	//file size and root hash might not be known, set them
	boost::uint64_t file_size = convert::bin_to_int<boost::uint64_t>(
		std::string(reinterpret_cast<char *>(buf.data()+3), 8));
	std::string root_hash = convert::bin_to_hex(std::string(
		reinterpret_cast<char *>(buf.data()+11), SHA1::bin_size));
	if(!slot_iter->set_unknown(Exchange.connection_ID, file_size, root_hash)){
		LOGGER << "error setting file size and root hash";
		Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::close_slot(buf[1])));
		return true;
	}

	//read bit field(s) (if any exist)
	bit_field tree_BF, file_BF;
	if(buf[2] == 1){
		//file bit_field
		boost::uint64_t file_block_count = file::calc_file_block_count(file_size);
		boost::uint64_t file_BF_size = bit_field::size_bytes(file_block_count, 1);
		assert(buf.size() == 31 + file_BF_size);
		file_BF.set_buf(buf.data() + 31, file_BF_size, file_block_count, 1);

		//unexpect previous have_file_block message with this slot (if exists)
		network::buffer unexpect_buf;
		unexpect_buf.append(protocol::have_file_block).append(buf[1]);
		Exchange.expect_anytime_erase(unexpect_buf);

		//expect have_hash_tree_block message
		Exchange.expect_anytime(boost::shared_ptr<message_tcp::base>(new
			message_tcp::have_file_block(boost::bind(&slot_manager::recv_have_file_block,
			this, _1), buf[1], file_block_count)));
	}else if(buf[2] == 2){
		//tree bit_field
		boost::uint64_t tree_block_count = hash_tree::calc_tree_block_count(file_size);
		boost::uint64_t tree_BF_size = bit_field::size_bytes(tree_block_count, 1);
		assert(buf.size() == 31 + tree_BF_size);
		tree_BF.set_buf(buf.data() + 31, tree_BF_size, tree_block_count, 1);

		//unexpect previous have_hash_tree_block message with this slot (if exists)
		network::buffer unexpect_buf;
		unexpect_buf.append(protocol::have_hash_tree_block).append(buf[1]);
		Exchange.expect_anytime_erase(unexpect_buf);

		//expect have_hash_tree_block message
		Exchange.expect_anytime(boost::shared_ptr<message_tcp::base>(new
			message_tcp::have_hash_tree_block(boost::bind(&slot_manager::recv_have_hash_tree_block,
			this, _1), buf[1], tree_block_count)));
	}else if(buf[2] == 3){
		//tree and file bit_field
		boost::uint64_t tree_block_count = hash_tree::calc_tree_block_count(file_size);
		boost::uint64_t tree_BF_size = bit_field::size_bytes(tree_block_count, 1);
		boost::uint64_t file_block_count = file::calc_file_block_count(file_size);
		boost::uint64_t file_BF_size = bit_field::size_bytes(file_block_count, 1);
		assert(buf.size() == 31 + tree_BF_size + file_BF_size);
		tree_BF.set_buf(buf.data() + 31, tree_BF_size, tree_block_count, 1);
		file_BF.set_buf(buf.data() + 31 + tree_BF_size, file_BF_size, file_block_count, 1);

		//unexpect previous have_hash_tree_block message with this slot (if exists)
		network::buffer unexpect_buf;
		unexpect_buf.append(protocol::have_hash_tree_block).append(buf[1]);
		Exchange.expect_anytime_erase(unexpect_buf);

		//expect have_hash_tree_block message
		Exchange.expect_anytime(boost::shared_ptr<message_tcp::base>(new
			message_tcp::have_hash_tree_block(boost::bind(&slot_manager::recv_have_hash_tree_block,
			this, _1), buf[1], tree_block_count)));

		//unexpect previous have_file_block message with this slot (if exists)
		unexpect_buf.clear();
		unexpect_buf.append(protocol::have_file_block).append(buf[1]);
		Exchange.expect_anytime_erase(unexpect_buf);

		//expect have_hash_tree_block message
		Exchange.expect_anytime(boost::shared_ptr<message_tcp::base>(new
			message_tcp::have_file_block(boost::bind(&slot_manager::recv_have_file_block,
			this, _1), buf[1], file_block_count)));
	}
	slot_iter->get_transfer()->outgoing_subscribe(Exchange.connection_ID, tree_BF, file_BF);

	//add outgoing slot
	std::pair<std::map<unsigned char, boost::shared_ptr<slot> >::iterator, bool>
		ret = Outgoing_Slot.insert(std::make_pair(buf[1], slot_iter.get()));
	if(ret.second == false){
		//host sent duplicate slot
		LOGGER << "violated protocol";
		return false;
	}
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
			if(iter_cur->second->get_transfer()){
				iter_cur->second->get_transfer()->incoming_unsubscribe(Exchange.connection_ID);
			}
			Incoming_Slot.erase(iter_cur++);
		}else{
			++iter_cur;
		}
	}
	for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Outgoing_Slot.begin(); iter_cur != Outgoing_Slot.end();)
	{
		if(iter_cur->second->hash() == hash){
			if(iter_cur->second->get_transfer()){
				iter_cur->second->get_transfer()->outgoing_unsubscribe(Exchange.connection_ID);
			}
			Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::close_slot(iter_cur->first)));
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
				Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::request_hash_tree_block(
					iter_cur->first, block_num, iter_cur->second->get_transfer()->tree_block_count())));
				boost::shared_ptr<message_tcp::composite> M_composite(new message_tcp::composite());
				M_composite->add(boost::shared_ptr<message_tcp::block>(new message_tcp::block(boost::bind(
					&slot_manager::recv_hash_tree_block, this, _1, iter_cur->first, block_num),
					block_size, iter_cur->second->get_transfer()->download_speed_calculator())));
				M_composite->add(boost::shared_ptr<message_tcp::error>(new message_tcp::error(
					boost::bind(&slot_manager::recv_request_block_failed, this, _1, iter_cur->first))));
				Exchange.expect_response(M_composite);
			}else if(iter_cur->second->get_transfer()->next_request_file(
				Exchange.connection_ID, block_num, block_size))
			{
				++pipeline_size;
				serviced_one = true;
				latest_slot = iter_cur->first;
				Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::request_file_block(
					iter_cur->first, block_num, iter_cur->second->get_transfer()->file_block_count())));
				boost::shared_ptr<message_tcp::composite> M_composite(new message_tcp::composite());
				M_composite->add(boost::shared_ptr<message_tcp::block>(new message_tcp::block(boost::bind(
					&slot_manager::recv_file_block, this, _1, iter_cur->first, block_num),
					block_size, iter_cur->second->get_transfer()->download_speed_calculator())));
				M_composite->add(boost::shared_ptr<message_tcp::error>(new message_tcp::error(
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
	boost::uint64_t block_num;
	for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
		iter_cur = Incoming_Slot.begin(), iter_end = Incoming_Slot.end();
		iter_cur != iter_end; ++iter_cur)
	{
		while(true){
			if(iter_cur->second->get_transfer()
				&& iter_cur->second->get_transfer()->next_have_tree(
				Exchange.connection_ID, block_num))
			{
				LOGGER << "tree: " << block_num;
				Exchange.send(boost::shared_ptr<message_tcp::base>(
					new message_tcp::have_hash_tree_block(iter_cur->first, block_num,
					iter_cur->second->get_transfer()->tree_block_count())));
			}else if(iter_cur->second->get_transfer()
				&& iter_cur->second->get_transfer()->next_have_file(
				Exchange.connection_ID, block_num))
			{
				LOGGER << "file: " << block_num;
				Exchange.send(boost::shared_ptr<message_tcp::base>(
					new message_tcp::have_file_block(iter_cur->first, block_num,
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
		Exchange.send(boost::shared_ptr<message_tcp::base>(new message_tcp::request_slot(
			slot_iter->hash())));
		boost::shared_ptr<message_tcp::composite> M_composite(new message_tcp::composite());
		M_composite->add(boost::shared_ptr<message_tcp::base>(new message_tcp::slot(
			boost::bind(&slot_manager::recv_slot, this, _1, slot_iter->hash()),
			slot_iter->hash())));
		M_composite->add(boost::shared_ptr<message_tcp::base>(new message_tcp::error(
			boost::bind(&slot_manager::recv_request_slot_failed, this, _1))));
		Exchange.expect_response(M_composite);
	}
}

void slot_manager::tick()
{
	close_complete();
	send_block_requests();
	send_have();
	send_slot_requests();
}
