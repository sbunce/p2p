#include "connection.hpp"

connection::connection(
	network::sock & S,
	share & Share_in
):
	IP(S.IP),
	port(S.port),
	Exchange(initial),
	blacklist_state(0),
	Slot_Manager(Share_in)
{
	if(database::table::blacklist::is_blacklisted(S.IP)){
		S.disconnect_flag = true;
	}else{
		//if outgoing connection start key exchange by sending p and r_A
		if(S.direction == network::sock::outgoing){
			Encryption.send_prime_and_local_result(S.send_buff);
			Exchange = sent_prime_and_local_result;
		}
	}
}

void make_requests(network::sock & S)
{

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
					database::table::blacklist::add(S.IP);
				}
				Exchange = complete;
			}
		}
	}else if(S.latest_recv != 0){
		Encryption.crypt_recv(S.recv_buff, S.recv_buff.size() - S.latest_recv);
		int initial_send_buff_size = S.send_buff.size();
		protocol::command_type_enum CT = protocol::command_type(S.recv_buff[0]);
		if(CT == protocol::slot_command){
			Slot_Manager.recv(S.recv_buff);
		}else{
			LOGGER << "invalid command from " << S.IP;
			database::table::blacklist::add(S.IP);
		}
		make_requests(S);
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
		int initial_send_buff_size = S.send_buff.size();
		make_requests(S);
		Encryption.crypt_send(S.send_buff, initial_send_buff_size);
	}

	if(database::table::blacklist::modified(blacklist_state)
		&& database::table::blacklist::is_blacklisted(IP))
	{
		S.disconnect_flag = true;
	}
}
