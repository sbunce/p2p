#include "connection.hpp"

connection::connection(
	network::proactor & Proactor_in,
	network::connection_info & CI
):
	IP(CI.IP),
	Proactor(Proactor_in),
	Slot_Manager(Exchange, CI),
	blacklist_state(0)
{
	if(database::table::blacklist::is_blacklisted(CI.IP)){
		Proactor.disconnect(CI.connection_ID);
	}else{
		//if outgoing connection start key exchange by sending p and r_A
		if(CI.direction == network::outgoing){
			network::buffer send_buf;
			Encryption.send_prime_and_local_result(send_buf);
			Proactor.write(CI.connection_ID, send_buf);
		}
		CI.recv_call_back = boost::bind(&connection::key_exchange_recv_call_back, this, _1);
	}
}

void connection::key_exchange_recv_call_back(network::connection_info & CI)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(CI.direction == network::outgoing){
		//expecting r_B
		if(CI.recv_buf.size() >= protocol::DH_KEY_SIZE){
			//remote result received, key exchange done
			Encryption.recv_remote_result(CI.recv_buf);
			CI.recv_call_back = boost::bind(&connection::recv_call_back, this, _1);
			CI.send_call_back = boost::bind(&connection::send_call_back, this, _1);

			//call new send call back to send initial requests
			CI.send_call_back(CI);
		}
	}else{
		//expecting p and r_A
		if(CI.recv_buf.size() >= protocol::DH_KEY_SIZE * 2){
			network::buffer send_buf;
			if(Encryption.recv_prime_and_remote_result(CI.recv_buf, send_buf)){
				//send r_B
				Proactor.write(CI.connection_ID, send_buf);

				//valid prime received, key exhange done
				CI.recv_call_back = boost::bind(&connection::recv_call_back, this, _1);
				CI.send_call_back = boost::bind(&connection::send_call_back, this, _1);

				//call new send call back to send initial requests
				CI.send_call_back(CI);
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

void connection::recv_call_back(network::connection_info & CI)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	Encryption.crypt_recv(CI.recv_buf, CI.recv_buf.size() - CI.latest_recv);
	while(boost::shared_ptr<exchange::message> M = Exchange.recv(CI)){
		if(M->recv_buf[0] == protocol::REQUEST_SLOT){
			if(!Slot_Manager.recv_REQUEST_SLOT(M)){
				database::table::blacklist::add(CI.IP);
				break;
			}
		}else if(M->recv_buf[0] == protocol::REQUEST_SLOT_FAILED){
			if(!Slot_Manager.recv_REQUEST_SLOT_FAILED(M)){
				database::table::blacklist::add(CI.IP);
				break;
			}
		}
	}
	if(database::table::blacklist::modified(blacklist_state)
		&& database::table::blacklist::is_blacklisted(CI.IP))
	{
		Proactor.disconnect(CI.connection_ID);
		CI.recv_call_back.clear();
		CI.send_call_back.clear();
	}
}

void connection::send_call_back(network::connection_info & CI)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	Slot_Manager.tick();
	if(CI.send_buf_size == 0){
		network::buffer send_buf = Exchange.send();
		Encryption.crypt_send(send_buf);
		Proactor.write(CI.connection_ID, send_buf);
	}
	if(database::table::blacklist::modified(blacklist_state)
		&& database::table::blacklist::is_blacklisted(CI.IP))
	{
		Proactor.disconnect(CI.connection_ID);
		CI.recv_call_back.clear();
		CI.send_call_back.clear();
	}
}
