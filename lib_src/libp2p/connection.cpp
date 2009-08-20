#include "connection.hpp"

connection::connection(network::sock & S):
	Exchange(INITIAL)
{
	//if outgoing connection start key exchange by sending p and r_A
	if(S.direction == network::OUTGOING){
		Encryption.send_prime_and_local_result(S.send_buff);
		Exchange = SENT_PRIME_AND_LOCAL_RESULT;
	}
}

void connection::recv_call_back(network::sock & S)
{
	if(Exchange != COMPLETE){
		if(Exchange == SENT_PRIME_AND_LOCAL_RESULT){
			//expecting r_B
			if(S.recv_buff.size() >= protocol::DH_KEY_SIZE){
				Encryption.recv_remote_result(S.recv_buff);
				Exchange = COMPLETE;
			}
		}else{
			//expecting p and r_A
			if(S.recv_buff.size() >= protocol::DH_KEY_SIZE * 2){
				if(!Encryption.recv_prime_and_remote_result(S.recv_buff, S.send_buff)){
					//remote host sent invalid prime
					database::table::blacklist::add(S.IP);
					S.disconnect_flag = true;
				}
				Exchange = COMPLETE;
			}
		}
	}else{
		//decrypt new incoming data
		Encryption.crypt_recv(S.recv_buff, S.recv_buff.size() - S.latest_recv);

		//store initial size so it is known how much to encrypt after appending
		int initial_send_buff_size = S.send_buff.size();

//do work here

		//encrypt from initial_send_buff_size to end of buffer
		Encryption.crypt_send(S.send_buff, initial_send_buff_size);
	}
}

void connection::send_call_back(network::sock & S)
{
	if(Exchange == COMPLETE){
		//store initial size so it is known how much to encrypt after appending
		int initial_send_buff_size = S.send_buff.size();

//do work here

		//encrypt from initial_send_buff_size to end of buffer
		Encryption.crypt_send(S.send_buff, initial_send_buff_size);
	}
}
