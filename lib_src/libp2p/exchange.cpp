#include "exchange.hpp"

static boost::shared_ptr<exchange::message> non_response(
	network::connection_info & CI, const unsigned size)
{
	if(CI.recv_buf.size() >= size){
		boost::shared_ptr<exchange::message> M(new exchange::message());
		M->recv_buf.append(CI.recv_buf.data(), size);
		return M;
	}else{
		return boost::shared_ptr<exchange::message>();
	}
}

boost::shared_ptr<exchange::message> exchange::recv(network::connection_info & CI)
{
	if(CI.recv_buf.empty()){
		return boost::shared_ptr<message>();
	}else if(CI.recv_buf[0] == protocol::REQUEST_SLOT){
		//expect at any time
		return non_response(CI, protocol::REQUEST_SLOT_SIZE);
	}else if(CI.recv_buf[0] == protocol::CLOSE_SLOT){
		//expect at any time
		return non_response(CI, protocol::CLOSE_SLOT_SIZE);
	}else if(CI.recv_buf[0] == protocol::SUBSCRIBE_HASH_TREE_CHANGES){
		//slot_manager checks to make sure slot open
		return non_response(CI, protocol::SUBSCRIBE_HASH_TREE_CHANGES_SIZE);
	}else if(CI.recv_buf[0] == protocol::SUBSCRIBE_FILE_CHANGES){
		//slot_manager checks to make sure slot open
		return non_response(CI, protocol::SUBSCRIBE_FILE_CHANGES_SIZE);
	}else if(CI.recv_buf[0] == protocol::HAVE_HASH_TREE_BLOCK){
		/*
		The slot_manager makes sure slot open. The slot makes sure we are
		subscribed to hash tree changes.
		*/
LOGGER; exit(1);
		//return non_response(CI, protocol::HAVE_HASH_TREE_BLOCK_SIZE);
	}else if(CI.recv_buf[0] == protocol::HAVE_FILE_BLOCK){
		/*
		The slot_manager makes sure slot open. The slot makes sure we are
		subscribed to file changes.
		*/
LOGGER; exit(1);
		//return non_response(CI, protocol::HAVE_FILE_BLOCK_SIZE);
	}else if(CI.recv_buf[0] == protocol::REQUEST_SLOT_FAILED
		|| CI.recv_buf[0] == protocol::REQUEST_BLOCK_HASH_TREE
		|| CI.recv_buf[0] == protocol::REQUEST_BLOCK_FILE
		|| CI.recv_buf[0] == protocol::FILE_REMOVED
		|| CI.recv_buf[0] == protocol::SLOT_ID
		|| CI.recv_buf[0] == protocol::BIT_FIELD
		|| CI.recv_buf[0] == protocol::BIT_FIELD_COMPLETE
		|| CI.recv_buf[0] == protocol::BLOCK
	){
		//message is a response to a message we sent
		if(Sent.empty()){
			//we weren't expecting any response
			database::table::blacklist::add(CI.IP);
			return boost::shared_ptr<message>();
		}else{
			boost::shared_ptr<message> M = Sent.front();
			Sent.pop_front();
			for(std::vector<std::pair<unsigned char, unsigned> >::iterator
				iter_cur = M->expected_response.begin(), iter_end = M->expected_response.end();
				iter_cur != iter_end; ++iter_cur)
			{
				if(CI.recv_buf[0] == iter_cur->first){
					M->recv_buf.append(CI.recv_buf.data(), iter_cur->second);
					return M;
				}
			}
			LOGGER << "unrecognized command " << static_cast<unsigned>(CI.recv_buf[0]);
			database::table::blacklist::add(CI.IP);
			return boost::shared_ptr<message>();
		}
	}else{
		LOGGER << "unrecognized command " << static_cast<unsigned>(CI.recv_buf[0]);
		database::table::blacklist::add(CI.IP);
		return boost::shared_ptr<message>();
	}
}

void exchange::schedule_send(boost::shared_ptr<message> & M)
{
	assert(M && !M->send_buf.empty());

//DEBUG, need prioritization here
	Send.push_back(M);
}

network::buffer exchange::send()
{
	if(Send.empty()){
		return network::buffer();
	}else{
		boost::shared_ptr<message> M = Send.front();
		Send.pop_front();
		if(!M->expected_response.empty()){
			Sent.push_back(M);
		}
		return M->send_buf;
	}
}
