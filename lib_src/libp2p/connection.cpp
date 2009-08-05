#include "connection.hpp"

connection::connection(
	network::sock & S,
	prime_generator & Prime_Generator_in
):
	Prime_Generator(Prime_Generator_in),
	Encryption(Prime_Generator),
	Exchange(INITIAL)
{
	if(S.direction == network::OUTGOING){
		Encryption.send_prime_and_local_result(S.send_buff);
		Exchange = SENT_PRIME_AND_LOCAL_RESULT;
	}
}

void connection::recv_call_back(network::sock & S)
{
	if(Exchange != COMPLETE){
		if(Exchange == SENT_PRIME_AND_LOCAL_RESULT){
			//expecting the remote_result
			if(S.recv_buff.size() >= protocol::DH_KEY_SIZE){
				Encryption.recv_remote_result(S.recv_buff);
				Exchange = COMPLETE;
			}
		}else{
			//expecting prime and remote_result
			if(S.recv_buff.size() >= protocol::DH_KEY_SIZE * 2){
				Encryption.recv_prime_and_remote_result(S.recv_buff, S.send_buff);
				Exchange = COMPLETE;
			}
		}
	}else{
		//decrypt new incoming data
		Encryption.crypt_recv(S.recv_buff, S.recv_buff.size() - S.latest_recv);

		//store initial size so we know how much to encrypt after appending
		int initial_send_buff_size = S.send_buff.size();

		LOGGER << S.recv_buff.str();

		Encryption.crypt_send(S.send_buff, initial_send_buff_size);
	}
}

void connection::send_call_back(network::sock & S)
{
	if(Exchange == COMPLETE){
		int initial_send_buff_size = S.send_buff.size();
		if(S.direction == network::INCOMING){
			static bool once = true;
			if(once){
				S.send_buff.append("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
				once = false;
			}
		}

		Encryption.crypt_send(S.send_buff, initial_send_buff_size);
	}
}
