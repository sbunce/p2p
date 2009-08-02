//custom
#include "../encryption.hpp"
#include "../settings.hpp"

int main()
{
	/*
	This test does Diffie-Helman-Merkle key-exchange. And then tests encrypting
	and decrypting a message.
	*/

	//setup database and make sure prime table clear
	path::unit_test_override("encryption.db");
	database::init init;
	database::table::prime::clear();

	//prime generator that the encryption class depends upon
	prime_generator Prime_Generator;
	Prime_Generator.start();

	//client/server encryption class
	encryption E_server(Prime_Generator), E_client(Prime_Generator);

	//client generates prime
	std::string prime_binary = E_client.get_prime();
	E_client.set_prime(prime_binary);

	//client sends prime and local result to server
	E_server.set_prime(prime_binary);
	E_server.set_remote_result(E_client.get_local_result());

	//server sends local result to client
	E_client.set_remote_result(E_server.get_local_result());

	//client encrypts message
	std::string client_buffer = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	E_client.crypt_send(client_buffer);

	if(client_buffer == "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ"){
		LOGGER; exit(1);
	}

	//server decrypts message from client
	E_server.crypt_recv(client_buffer);

	if(client_buffer != "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ"){
		LOGGER; exit(1);
	}

	//now server encrypts message
	std::string server_buffer = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	E_server.crypt_send(server_buffer);

	if(server_buffer == "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ"){
		LOGGER; exit(1);
	}

	//client decrypts message from server
	E_client.crypt_recv(server_buffer);

	if(server_buffer != "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ"){
		LOGGER; exit(1);
	}
	Prime_Generator.stop();
}
