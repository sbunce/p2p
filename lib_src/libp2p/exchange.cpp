#include "exchange.hpp"

static boost::shared_ptr<exchange::message> non_response(
	network::connection_info & CI, const unsigned size)
{
	if(CI.recv_buf.size() >= size){
		boost::shared_ptr<exchange::message> M(new exchange::message());
		M->recv_buf.append(CI.recv_buf.data(), size);
		CI.recv_buf.erase(0, size);
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
		return non_response(CI, protocol::REQUEST_SLOT_SIZE);
	}else if(CI.recv_buf[0] == protocol::CLOSE_SLOT){
		return non_response(CI, protocol::CLOSE_SLOT_SIZE);
	}else if(CI.recv_buf[0] == protocol::HAVE_HASH_TREE_BLOCK){

LOGGER << "stub, need to memorize message size"; exit(1);
	}else if(CI.recv_buf[0] == protocol::HAVE_FILE_BLOCK){

LOGGER << "stub, need to memorize message size"; exit(1);
	}else if(CI.recv_buf[0] == protocol::ERROR ||
		CI.recv_buf[0] == protocol::SLOT ||
		CI.recv_buf[0] == protocol::REQUEST_HASH_TREE_BLOCK ||
		CI.recv_buf[0] == protocol::REQUEST_FILE_BLOCK ||
		CI.recv_buf[0] == protocol::BLOCK
	){
		if(Sent.empty()){
			LOGGER << "unexpected response";
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
					CI.recv_buf.erase(0, iter_cur->second);
					return M;
				}
			}
			LOGGER << "unrecognized command";
			database::table::blacklist::add(CI.IP);
			return boost::shared_ptr<message>();
		}
	}else{
		LOGGER << "unrecognized command";
		database::table::blacklist::add(CI.IP);
		return boost::shared_ptr<message>();
	}
}

void exchange::schedule_send(boost::shared_ptr<message> & M)
{
	assert(M && !M->send_buf.empty());
//DEBUG, prioritizion will be done here

//DEBUG, check for SUBSCRIBE_* and memorize length of HAVE_* responses
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
