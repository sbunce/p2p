#include "connection.hpp"

connection::connection(
	network::proactor & Proactor_in,
	network::connection_info & CI
):
	Proactor(Proactor_in),
	Slot_Manager(Proactor_in, CI),
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
				//invalid prime, blacklist IP
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
	//decrypt incoming data
	Encryption.crypt_recv(CI.recv_buf, CI.recv_buf.size() - CI.latest_recv);

	network::buffer send_buf;

/*
	//DEBUG, BEGIN TEST CODE
	LOGGER << CI.recv_buf.str();
	//DEBUG, END TEST CODE
*/

	//process messages in buffer
	while(true){
		if(slot_manager::is_slot_command(CI.recv_buf[0])){
			if(!Slot_Manager.recv(CI)){
				break;
			}
		}else{
			database::table::blacklist::add(CI.IP);
			break;
		}
	}

	if(!send_buf.empty()){
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

void connection::send_call_back(network::connection_info & CI)
{
	network::buffer send_buf;

/*
	//DEBUG, BEGIN TEST CODE
	if(CI.direction == network::outgoing){
		static bool sent = false;
		if(!sent){
			send_buf.append("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
			sent = true;
		}
	}
	//DEBUG, END TEST CODE
*/

	//Slot_Manager.send(send_buf);

	//encrypt what was appended to send_buff
	if(!send_buf.empty()){
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
