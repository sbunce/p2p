#include "connection.hpp"

connection::connection(network::sock & S):
	IP(S.IP),
	port(S.port),
	Exchange(initial),
	blacklist_state(-1)
{
	//if outgoing connection start key exchange by sending p and r_A
	if(S.direction == network::sock::outgoing){
		Encryption.send_prime_and_local_result(S.send_buff);
		Exchange = sent_prime_and_local_result;
	}
}

void connection::recv_call_back(network::sock & S)
{
	if(Exchange != complete){
		if(Exchange == sent_prime_and_local_result){
			//expecting r_B
			if(S.recv_buff.size() >= protocol::DH_KEY_SIZE){
				Encryption.recv_remote_result(S.recv_buff);
				Exchange = complete;
			}
		}else{
			//expecting p and r_A
			if(S.recv_buff.size() >= protocol::DH_KEY_SIZE * 2){
				if(!Encryption.recv_prime_and_remote_result(S.recv_buff, S.send_buff)){
					//remote host sent invalid prime
					database::table::blacklist::add(S.IP);
				}
				Exchange = complete;
			}
		}
	}else{
		//decrypt new incoming data
		Encryption.crypt_recv(S.recv_buff, S.recv_buff.size() - S.latest_recv);

		//store initial size so it is known how much to encrypt after appending
		int initial_send_buff_size = S.send_buff.size();

		if(!S.recv_buff.empty() && !protocol::valid_command(S.recv_buff[0])){
			LOGGER << "invalid command from " << S.IP;
			database::table::blacklist::add(S.IP);
		}

//do stuff here

		//encrypt from initial_send_buff_size to end of buffer
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
	if(Exchange == complete){
		//store initial size so it is known how much to encrypt after appending
		int initial_send_buff_size = S.send_buff.size();

//do stuff here

		//encrypt from initial_send_buff_size to end of buffer
		Encryption.crypt_send(S.send_buff, initial_send_buff_size);
	}

	if(database::table::blacklist::modified(blacklist_state)
		&& database::table::blacklist::is_blacklisted(IP))
	{
		S.disconnect_flag = true;
	}
}
