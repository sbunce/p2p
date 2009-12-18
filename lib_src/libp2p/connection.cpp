#include "connection.hpp"

connection::connection(
	network::proactor & Proactor_in,
	network::connection_info & CI
):
	IP(CI.IP),
	connection_ID(CI.connection_ID),
	Proactor(Proactor_in),
	Slot_Manager(boost::bind(&connection::send, this, _1, _2)),
	blacklist_state(-1)
{
	//start key exchange
	if(CI.direction == network::outgoing){
		send(
			boost::shared_ptr<message::base>(new message::key_exchange_p_rA(Encryption)),
			boost::shared_ptr<message::base>(new message::key_exchange_rB())
		);
	}else{
		send(
			boost::shared_ptr<message::base>(),
			boost::shared_ptr<message::base>(new message::key_exchange_p_rA())
		);
	}

	//register possible incoming messages
	Non_Response.push_back(boost::shared_ptr<message::base>(
		new message::request_slot()));

	CI.recv_call_back = boost::bind(&connection::recv_call_back, this, _1);
	CI.recv_call_back(CI);
}

void connection::recv_call_back(network::connection_info & CI)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(Encryption.ready()){
		Encryption.crypt_recv(CI.recv_buf, CI.recv_buf.size() - CI.latest_recv);
	}

	while(!CI.recv_buf.empty()){
		if(!Expected.empty() && Expected.front().response->expects(CI)){
			//next message is expected response
			if(Expected.front().response->recv(CI)){
				//complete response received
				message::pair MP = Expected.front();
				Expected.pop_front();
				if(MP.response->type() == message::key_exchange_rB_t){
					Encryption.recv_rB(MP.response->buf());
					//unencrypt any remaining buffer
					Encryption.crypt_recv(CI.recv_buf);
					send_initial();
				}else if(MP.response->type() == message::key_exchange_p_rA_t){
					Encryption.recv_p_rA(MP.response->buf());
					send(
						boost::shared_ptr<message::base>(new message::key_exchange_rB(Encryption)),
						boost::shared_ptr<message::base>()
					);
					//unencrypt any remaining buffer
					Encryption.crypt_recv(CI.recv_buf);
					send_initial();
				}else if(MP.response->type() == message::initial_t){
					peer_ID = convert::bin_to_hex(reinterpret_cast<char *>(
						MP.response->buf().data()), SHA1::bin_size);
					LOGGER << CI.IP << " " << CI.port << " peer_ID: " << peer_ID;
					Slot_Manager.resume(peer_ID);
				}else if(MP.request && MP.request->type() == message::request_slot_t){
					if(MP.response->type() == message::slot_t){
						Slot_Manager.recv_slot(MP);
					}else if(MP.response->type() == message::error_t){
						Slot_Manager.recv_request_slot_failed(MP);
					}
				}else{
					LOGGER << "stub: unhandled message " << MP.response->type(); exit(1);
				}
				//check for more message in CI.recv_buf
				continue;
			}else{
				//full message not yet received
				break;
			}
		}

		//check for message that's not a response
		bool expects = false;
		for(std::vector<boost::shared_ptr<message::base> >::iterator
			iter_cur = Non_Response.begin(), iter_end = Non_Response.end();
			iter_cur != iter_end; ++iter_cur)
		{
			if((*iter_cur)->expects(CI)){
				expects = true;
				if((*iter_cur)->recv(CI)){
					//complete message received
					if((*iter_cur)->type() == message::request_slot_t){
						Slot_Manager.recv_request_slot(*iter_cur);
					}
					//clear message for reuse
					(*iter_cur)->buf().clear();
				}
				break;
			}
		}
		if(!expects){
			LOGGER << "unrecognized message";
			database::table::blacklist::add(CI.IP);
			break;
		}
	}

	//receive may have caused host to be blacklisted
	if(database::table::blacklist::modified(blacklist_state)
		&& database::table::blacklist::is_blacklisted(CI.IP))
	{
		LOGGER << "disconnecting blacklisted " << CI.IP;
		Proactor.disconnect(CI.connection_ID);
		CI.recv_call_back.clear();
		return;
	}
}

void connection::send(boost::shared_ptr<message::base> M_send,
	boost::shared_ptr<message::base> M_response)
{
	if(M_send){
		assert(!M_send->buf().empty());
		if(M_send->type() == message::key_exchange_p_rA_t ||
			M_send->type() == message::key_exchange_rB_t)
		{
			Proactor.send(connection_ID, M_send->buf());
		}else{
			//copy buffer to leave original unencrypted
			network::buffer buf = M_send->buf();
			Encryption.crypt_send(buf);
			Proactor.send(connection_ID, buf);
		}
	}
	if(M_response){
		Expected.push_back(message::pair(M_send, M_response));
	}
}

void connection::send_initial()
{
	std::string peer_ID = database::table::prefs::get_peer_ID();
	send(
		boost::shared_ptr<message::base>(new message::initial(peer_ID)),
		boost::shared_ptr<message::base>(new message::initial())
	);
}
