#include "connection.hpp"

connection::connection(
	network::proactor & Proactor_in,
	network::connection_info & CI
):
	IP(CI.IP),
	Proactor(Proactor_in),
	Slot_Manager(Send),
	blacklist_state(0)
{
	if(database::table::blacklist::is_blacklisted(CI.IP)){
		Proactor.disconnect(CI.connection_ID);
	}else{
		//start key exchange by sending p and r_A
		if(CI.direction == network::outgoing){
			network::buffer send_buf;
			Encryption.send_prime_and_local_result(send_buf);
			Proactor.send(CI.connection_ID, send_buf);
		}
		CI.recv_call_back = boost::bind(&connection::key_exchange_recv_call_back, this, _1);
	}
}

void connection::key_exchange_recv_call_back(network::connection_info & CI)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(CI.direction == network::outgoing){
		if(CI.recv_buf.size() >= protocol::DH_KEY_SIZE){
			//recv r_B
			Encryption.recv_remote_result(CI.recv_buf);

			//remote result received, key exchange done
			initial_send(CI);
			CI.recv_call_back = boost::bind(&connection::initial_recv_call_back, this, _1);
			CI.latest_recv = CI.recv_buf.size();
			CI.recv_call_back(CI); //initial may have already arrived
		}
	}else{
		if(CI.recv_buf.size() >= protocol::DH_KEY_SIZE * 2){
			//recv p and r_A
			network::buffer send_buf;
			if(Encryption.recv_prime_and_remote_result(CI.recv_buf, send_buf)){
				//send r_B
				Proactor.send(CI.connection_ID, send_buf);

				//valid prime received, key exhange done
				initial_send(CI);
				CI.recv_call_back = boost::bind(&connection::initial_recv_call_back, this, _1);
			}else{
				//invalid prime
				database::table::blacklist::add(CI.IP);
				Proactor.disconnect(CI.connection_ID);
				CI.recv_call_back.clear();
				CI.send_call_back.clear();
			}
		}
	}
}

void connection::initial_recv_call_back(network::connection_info & CI)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	Encryption.crypt_recv(CI.recv_buf, CI.recv_buf.size() - CI.latest_recv);
	if(CI.recv_buf.size() >= SHA1::bin_size){
		peer_ID = convert::bin_to_hex(
			reinterpret_cast<const char *>(CI.recv_buf.data()), SHA1::bin_size);
		CI.recv_buf.erase(0, SHA1::bin_size);
		LOGGER << CI.IP << " " << CI.port << " peer_ID: " << peer_ID;
		Slot_Manager.resume(CI, peer_ID);
		CI.recv_call_back = boost::bind(&connection::recv_call_back, this, _1);
		CI.latest_recv = 0;
		CI.recv_call_back(CI);
	}
}

void connection::initial_send(network::connection_info & CI)
{
	network::buffer send_buf;
	send_buf.append(convert::hex_to_bin(database::table::prefs::get_peer_ID()));
	Encryption.crypt_send(send_buf);
	Proactor.send(CI.connection_ID, send_buf);
}

void connection::recv_call_back(network::connection_info & CI)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	Encryption.crypt_recv(CI.recv_buf, CI.recv_buf.size() - CI.latest_recv);
	while(!CI.recv_buf.empty()){
		if(CI.recv_buf[0] == protocol::REQUEST_SLOT){
			if(!Slot_Manager.recv_request_slot(CI)){
				break;
			}
		}else if(CI.recv_buf[0] == protocol::SLOT){
			if(!Slot_Manager.recv_slot(CI)){
				break;
			}
		}else{
////////////////////////////////////////////////////////////////
//DEBUG, proactor.write's in Slot_Manager need to use encryption
			LOGGER << "unhandled command";
			database::table::blacklist::add(CI.IP);
			break;
		}
	}

	if(database::table::blacklist::modified(blacklist_state)
		&& database::table::blacklist::is_blacklisted(CI.IP))
	{
		Proactor.disconnect(CI.connection_ID);
		CI.recv_call_back.clear();
		return;
	}

	//handle sends
	while(!Send.empty()){
		assert(!Send.front().empty());
		if(Send.front()[0] == protocol::REQUEST_SLOT){
			Sent.push_back(Send.front());
		}
		Encryption.crypt_send(Send.front());
		Proactor.send(CI.connection_ID, Send.front());
		Send.pop_front();
	}
}
