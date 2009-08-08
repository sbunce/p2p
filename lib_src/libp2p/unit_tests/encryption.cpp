//custom
#include "../encryption.hpp"
#include "../settings.hpp"

//copies one buffer to another, simulates sending over network
void send(network::buffer & source, network::buffer & destination)
{
	destination.append(source.data(), source.size());
	source.clear();
}

int main()
{
	path::unit_test_override("encryption.db");
	prime_generator Prime_Generator;
	Prime_Generator.start();

	//Host_A will connect to Host_B.
	encryption Host_A_Encryption(Prime_Generator);
	network::buffer Host_A_send_buff;
	network::buffer Host_A_recv_buff;

	encryption Host_B_Encryption(Prime_Generator);
	network::buffer Host_B_send_buff;
	network::buffer Host_B_recv_buff;

	//Host_A sends prime and local_result.
	Host_A_Encryption.send_prime_and_local_result(Host_A_send_buff);
	send(Host_A_send_buff, Host_B_recv_buff);

	//Host_B receives prime and remote_result. Host_B sends local_result.
	Host_B_Encryption.recv_prime_and_remote_result(Host_B_recv_buff, Host_B_send_buff);
	send(Host_B_send_buff, Host_A_recv_buff);

	//Host_A receives remote_result.
	Host_A_Encryption.recv_remote_result(Host_A_recv_buff);

	//all buffers should be empty at this point
	assert(Host_A_send_buff.empty());
	assert(Host_A_recv_buff.empty());
	assert(Host_B_send_buff.empty());
	assert(Host_B_recv_buff.empty());

	/*
	Both hosts are ready to encrypt/decrypt. Test sending a message from Host_A
	to Host_B.
	*/
	std::string message = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	Host_A_send_buff.append(message);
	Host_A_Encryption.crypt_send(Host_A_send_buff);
	send(Host_A_send_buff, Host_B_recv_buff);
	Host_B_Encryption.crypt_recv(Host_B_recv_buff);
	if(Host_B_recv_buff != message){
		LOGGER; exit(1);
	}

	//test sending a message from Host_B to Host_A
	Host_B_send_buff.append(message);
	Host_B_Encryption.crypt_send(Host_B_send_buff);
	send(Host_B_send_buff, Host_A_recv_buff);
	Host_A_Encryption.crypt_recv(Host_A_recv_buff);
	if(Host_A_recv_buff != message){
		LOGGER; exit(1);
	}

	Prime_Generator.stop();
}
