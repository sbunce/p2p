#include "slot_manager.hpp"

slot_manager::slot_manager(network::connection_info & CI):
	connection_ID(CI.connection_ID),
	IP(CI.IP),
	port(CI.port),
	share_slot_state(0)
{
	sync_slots();
}

bool slot_manager::is_slot_command(const unsigned char command)
{
	return command <= 12;
}

bool slot_manager::recv(network::connection_info & CI)
{
	assert(!CI.recv_buf.empty() && is_slot_command(CI.recv_buf[0]));

}

void slot_manager::send(network::buffer & send_buf)
{
	
}

void slot_manager::send_close_slot(const unsigned char slot_ID)
{
	boost::shared_ptr<message> M(new message());
	M->send_buf.append(protocol::CLOSE_SLOT).append(slot_ID);
	Send_Queue.push_back(M);
}

void slot_manager::send_request_slot(const std::string & hash)
{
	boost::shared_ptr<message> M(new message());
	M->send_buf.append(protocol::REQUEST_SLOT).append(convert::hex_to_bin(hash));
	M->expected_response.push_back(std::make_pair(protocol::SLOT_ID,
		protocol::SLOT_ID_SIZE));
	M->expected_response.push_back(std::make_pair(protocol::REQUEST_SLOT_FAILED,
		protocol::REQUEST_SLOT_FAILED_SIZE));
	Send_Queue.push_back(M);
}

void slot_manager::send_request_slot_failed()
{
	boost::shared_ptr<message> M(new message());
	M->send_buf.append(protocol::REQUEST_SLOT_FAILED);
	Send_Queue.push_back(M);
}

void slot_manager::send_request_block(
	std::pair<unsigned char, boost::shared_ptr<slot> > & P)
{
	//request hash tree blocks first
	boost::uint64_t block;
	if(P.second->Hash_Tree.Block_Request.next_request(connection_ID, block)){
		boost::shared_ptr<message> M(new message());
		M->Slot = P.second;
		M->send_buf.append(protocol::REQUEST_BLOCK_HASH_TREE).append(P.first)
			.append(convert::encode_VLI(block, P.second->Hash_Tree.tree_block_count));
		M->expected_response.push_back(std::make_pair(protocol::BLOCK,
			protocol::BLOCK_SIZE(P.second->Hash_Tree.block_size(block))));
		M->expected_response.push_back(std::make_pair(protocol::FILE_REMOVED,
			protocol::FILE_REMOVED_SIZE));
		Send_Queue.push_back(M);
	}else if(P.second->File.Block_Request.next_request(connection_ID, block)){
		boost::shared_ptr<message> M(new message());
		M->Slot = P.second;
		M->send_buf.append(protocol::REQUEST_BLOCK_FILE).append(P.first)
			.append(convert::encode_VLI(block, P.second->File.block_count));
		M->expected_response.push_back(std::make_pair(protocol::BLOCK,
			protocol::BLOCK_SIZE(P.second->File.block_size(block))));
		M->expected_response.push_back(std::make_pair(protocol::FILE_REMOVED,
			protocol::FILE_REMOVED_SIZE));
		Send_Queue.push_back(M);
	}
}

void slot_manager::send_file_removed()
{
	boost::shared_ptr<message> M(new message());
	M->send_buf.append(protocol::FILE_REMOVED);
	Send_Queue.push_back(M);
}

void slot_manager::send_slot_ID(std::pair<unsigned char, boost::shared_ptr<slot> > & P)
{
	boost::shared_ptr<message> M(new message());

///////////////
//DEBUG, need to store root_hash in file_info within share.

	Send_Queue.push_back(M);
}

void slot_manager::sync_slots()
{
	//only sync with share if slot has been modified
	if(!share::singleton().slot_modified(share_slot_state)){
		return;
	}

	for(share::slot_iterator iter_cur = share::singleton().begin_slot(),
		iter_end = share::singleton().end_slot(); iter_cur != iter_end; ++iter_cur)
	{
		//true if slot exists in this slot_manager
		bool known = Known_Slot.find(iter_cur.get()) != Known_Slot.end();
		//true if this IP/port has file the slot is for
		bool has_file = iter_cur->has_file(IP, port);

		if(known && !has_file){
			//this host was removed as a source for the file
			/* Step 1
			Set Incoming_Slot shared_ptr to empty. The next request the remote host
			makes will result in a FILE_REMOVED response.
			*/
			for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
				IS_iter_cur = Incoming_Slot.begin(), IS_iter_end = Incoming_Slot.end();
				IS_iter_cur != IS_iter_end; ++IS_iter_cur)
			{
				if(IS_iter_cur->second == iter_cur.get()){
					IS_iter_cur->second = boost::shared_ptr<slot>();
					break;
				}
			}
			/* Step 2
			Send CLOSE_SLOT for Outgoing_Slot, remove element.
			Note: The CLOSE_SLOT message put in the Send_Queue won't be removed by
				step 6 since it doesn't expect a response (Slot shared_ptr in
				message empty).
			*/
			for(std::map<unsigned char, boost::shared_ptr<slot> >::iterator
				OS_iter_cur = Outgoing_Slot.begin(), OS_iter_end = Outgoing_Slot.end();
				OS_iter_cur != OS_iter_end; ++OS_iter_cur)
			{
				if(OS_iter_cur->second == iter_cur.get()){
					send_close_slot(OS_iter_cur->first);
					Outgoing_Slot.erase(OS_iter_cur);
					break;
				}
			}
			/* Step 3
			Remove element in Pending_Slot_Request so slot request is not made.
			*/
			for(std::deque<boost::shared_ptr<slot> >::iterator
				PSR_iter_cur = Pending_Slot_Request.begin(), PSR_iter_end = Pending_Slot_Request.end();
				PSR_iter_cur != PSR_iter_end; ++PSR_iter_cur)
			{
				if(*PSR_iter_cur == iter_cur.get()){
					Pending_Slot_Request.erase(PSR_iter_cur);
					break;
				}
			}
			/* Step 4
			Set element in Slot_Request to empty. If the remote host responds with
			a SLOT_ID we will send a CLOSE_SLOT. If the remote host responds with a
			REQUEST_SLOT_FAILED we do nothing.
			*/
			for(std::deque<boost::shared_ptr<slot> >::iterator
				SR_iter_cur = Slot_Request.begin(), SR_iter_end = Slot_Request.end();
				SR_iter_cur != SR_iter_end; ++SR_iter_cur)
			{
				if(*SR_iter_cur == iter_cur.get()){
					*SR_iter_cur = boost::shared_ptr<slot>();
					break;
				}
			}
			/* Step 5
			Remove elements in Send_Queue to stop pending requests for the slot.
			*/
			for(std::deque<boost::shared_ptr<message> >::iterator
				SQ_iter_cur = Send_Queue.begin(), SQ_iter_end = Send_Queue.end();
				SQ_iter_cur != SQ_iter_end;)
			{
				if((*SQ_iter_cur)->Slot == iter_cur.get()){
					Send_Queue.erase(SQ_iter_cur);
					//iterators invalidated after erase
					SQ_iter_cur = Send_Queue.begin();
					SQ_iter_end = Send_Queue.end();
				}else{
					++SQ_iter_cur;
				}
			}
			/* Step 6
			Set elements in Sent_Queue to empty so we ignore responses for the
			closed slot.
			*/
			for(std::deque<boost::shared_ptr<message> >::iterator
				SQ_iter_cur = Sent_Queue.begin(), SQ_iter_end = Sent_Queue.end();
				SQ_iter_cur != SQ_iter_end; ++SQ_iter_cur)
			{
				if((*SQ_iter_cur)->Slot == iter_cur.get()){
					(*SQ_iter_cur)->Slot = boost::shared_ptr<slot>();
				}
			}
			//Step 7
			Known_Slot.erase(iter_cur.get());
		}

		if(!known && has_file && !iter_cur->complete()){
			//file needs to be downloaded
			LOGGER << "opening slot " << iter_cur->File.name() << " to " << IP << ":" << port;
			Pending_Slot_Request.push_back(iter_cur.get());
			Known_Slot.insert(iter_cur.get());
		}
	}
	share::singleton().remove_unused_slots();
}
