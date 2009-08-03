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

	//Host_0 will connect to Host_1.
	encryption Host_0_Encryption(Prime_Generator);
	network::buffer Host_0_send_buff;
	network::buffer Host_0_recv_buff;

	encryption Host_1_Encryption(Prime_Generator);
	network::buffer Host_1_send_buff;
	network::buffer Host_1_recv_buff;

	//Host_0 sends prime and local_result.
	Host_0_Encryption.send_prime_and_local_result(Host_0_send_buff);
	send(Host_0_send_buff, Host_1_recv_buff);

	//Host_1 receives prime and remote_result. Host_1 sends local_result.
	Host_1_Encryption.recv_prime_and_remote_result(Host_1_recv_buff, Host_1_send_buff);
	send(Host_1_send_buff, Host_0_recv_buff);

	//Host_0 receives remote_result.
	Host_0_Encryption.recv_remote_result(Host_0_recv_buff);

	//all buffers should be empty at this point
	assert(Host_0_send_buff.empty());
	assert(Host_0_recv_buff.empty());
	assert(Host_1_send_buff.empty());
	assert(Host_1_recv_buff.empty());

	/*
	Both hosts are ready to encrypt/decrypt. Test sending a message from Host_0
	to Host_1.
	*/
	std::string message = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	Host_0_send_buff.append(message);
	Host_0_Encryption.crypt_send(Host_0_send_buff);
	send(Host_0_send_buff, Host_1_recv_buff);
	Host_1_Encryption.crypt_recv(Host_1_recv_buff);
	if(Host_1_recv_buff != message){
		LOGGER; exit(1);
	}

	//test sending a message from Host_1 to Host_0
	Host_1_send_buff.append(message);
	Host_1_Encryption.crypt_send(Host_1_send_buff);
	send(Host_1_send_buff, Host_0_recv_buff);
	Host_0_Encryption.crypt_recv(Host_0_recv_buff);
	if(Host_0_recv_buff != message){
		LOGGER; exit(1);
	}

	Prime_Generator.stop();
}
