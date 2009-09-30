#include "connection.hpp"

connection::connection(
	network::sock & S,
	share & Share_in
):
	IP(S.IP),
	port(S.port),
	blacklist_state(0),
	Slot_Manager(Share_in)
{
	if(database::table::blacklist::is_blacklisted(S.IP)){
		S.disconnect_flag = true;
	}else{
		S.recv_call_back = boost::bind(&connection::key_exchange_recv_call_back, this, _1);
		S.send_call_back = boost::bind(&connection::key_exchange_send_call_back, this, _1);

		//if outgoing connection start key exchange by sending p and r_A
		if(S.direction == network::sock::outgoing){
			Encryption.send_prime_and_local_result(S.send_buff);
		}
	}
}

void connection::key_exchange_recv_call_back(network::sock & S)
{
	if(S.direction == network::sock::outgoing){
		//expecting r_B
		if(S.recv_buff.size() >= protocol::DH_KEY_SIZE){
			//remote result received, key exchange done
			Encryption.recv_remote_result(S.recv_buff);
			S.recv_call_back = boost::bind(&connection::recv_call_back, this, _1);
			S.send_call_back = boost::bind(&connection::send_call_back, this, _1);

			//call new send call back to send initial requests
			S.send_call_back(S);
		}
	}else{
		//expecting p and r_A
		if(S.recv_buff.size() >= protocol::DH_KEY_SIZE * 2){
			if(Encryption.recv_prime_and_remote_result(S.recv_buff, S.send_buff)){
				//valid prime received, key exhange done
				S.recv_call_back = boost::bind(&connection::recv_call_back, this, _1);
				S.send_call_back = boost::bind(&connection::send_call_back, this, _1);

				//call new send call back to send initial requests
				S.send_call_back(S);
			}else{
				//invalid prime, blacklist IP
				database::table::blacklist::add(S.IP);
				S.disconnect_flag = true;
			}
		}
	}
}

void connection::key_exchange_send_call_back(network::sock & S)
{
	//nothing to do after sending for key exchange
}

void connection::recv_call_back(network::sock & S)
{
	//latest_recv is zero when a call back has been forced
	if(S.latest_recv != 0){
		//decrypt incoming data
		Encryption.crypt_recv(S.recv_buff, S.recv_buff.size() - S.latest_recv);

		//save initial send_buff size so we know how much was appended
		int initial_send_buff_size = S.send_buff.size();

/*
		protocol::type Type = protocol::command_type(S.recv_buff[0]);
		if(Type == protocol::slot_command){
			Slot_Manager.recv(S.recv_buff);
		}else{
			LOGGER << "invalid command from " << S.IP;
			database::table::blacklist::add(S.IP);
		}
*/

//DEBUG, test code
//LOGGER << S.recv_buff.str();

		//encrypt what was appended to send_buff
		Encryption.crypt_send(S.send_buff, initial_send_buff_size);
	}

	if(database::table::blacklist::modified(blacklist_state)
		&& database::table::blacklist::is_blacklisted(IP))
	{
		S.disconnect_flag = true;
	}
}

void connection::send_call_back(network::sock & S)
{
	//save initial send_buff size so we know how much was appended
	int initial_send_buff_size = S.send_buff.size();

/*
//DEBUG, test code
if(S.direction == network::sock::outgoing){
	static bool sent = false;
	if(!sent){
		S.send_buff.append("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
		sent = true;
	}
}
*/
	//encrypt what was appended to send_buff
	Encryption.crypt_send(S.send_buff, initial_send_buff_size);

	if(database::table::blacklist::modified(blacklist_state)
		&& database::table::blacklist::is_blacklisted(IP))
	{
		S.disconnect_flag = true;
	}
}
