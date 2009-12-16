#include "connection.hpp"

connection::connection(
	network::proactor & Proactor_in,
	network::connection_info & CI
):
	IP(CI.IP),
	connection_ID(CI.connection_ID),
	Proactor(Proactor_in),
	Slot_Manager(boost::bind(&connection::send, this, _1)),
	blacklist_state(-1)
{
	if(CI.direction == network::outgoing){
		boost::shared_ptr<message> M(new key_exchange_p_rA());
		M->buf = Encryption.send_p_rA();
		send(M);
	}else{
		Expected.push_back(message_pair(
			boost::shared_ptr<message>(),
			boost::shared_ptr<message>(new key_exchange_p_rA())
		));
	}
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
			if(Expected.front().response->parse(CI)){
				//complete response received
				message_pair MP = Expected.front();
				Expected.pop_front();
				if(MP.response->type() == message::key_exchange_rB){
					Encryption.recv_rB(MP.response->buf);
					Encryption.crypt_recv(CI.recv_buf);
					send_initial();
				}else if(MP.response->type() == message::key_exchange_p_rA){
					Encryption.recv_p_rA(MP.response->buf);
					boost::shared_ptr<message> M(new key_exchange_rB());
					M->buf = Encryption.send_rB();
					send(M);
					Encryption.crypt_recv(CI.recv_buf);
					send_initial();
				}else if(MP.response->type() == message::initial){
					peer_ID = convert::bin_to_hex(
						reinterpret_cast<const char *>(MP.response->buf.data()), SHA1::bin_size);
					LOGGER << CI.IP << " " << CI.port << " peer_ID: " << peer_ID;
					//Slot_Manager.resume(CI, peer_ID);
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

		LOGGER << "unrecognized message";
		database::table::blacklist::add(CI.IP);
		break;
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

void connection::send(boost::shared_ptr<message> M)
{
	if(M->type() == message::key_exchange_p_rA){
		Expected.push_back(message_pair(
			boost::shared_ptr<message>(),
			boost::shared_ptr<message>(new key_exchange_rB())
		));
	}else if(M->type() == message::initial){
		Expected.push_back(message_pair(
			boost::shared_ptr<message>(),
			boost::shared_ptr<message>(new initial())
		));
	}
	//copy buffer to leave original unencrypted
	network::buffer buf = M->buf;
	if(M->type() != message::key_exchange_p_rA &&
		M->type() != message::key_exchange_rB)
	{
		Encryption.crypt_send(buf);
	}
	Proactor.send(connection_ID, buf);
}

void connection::send_initial()
{
	boost::shared_ptr<message> M(new initial());
	M->buf.append(convert::hex_to_bin(database::table::prefs::get_peer_ID()));
	send(M);
}
