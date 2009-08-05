//NOT-THREADSAFE
#ifndef H_ENCRYPTION
#define H_ENCRYPTION

//custom
#include "prime_generator.hpp"
#include "protocol.hpp"
#include "settings.hpp"

//include
#include <boost/utility.hpp>
#include <KISS.hpp>
#include <network/network.hpp>
#include <random.hpp>
#include <tommath/mpint.hpp>

//std
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>

class encryption : private boost::noncopyable
{
public:
	//size of the key exchanged (bytes)
	static const int DHM_KEY_SIZE = 16;

	encryption(prime_generator & Prime_Generator_in);

	/*
	Used to encrypt/decrypt buffers.
	Decrypts buffer from index to end of buffer.
	Precondition: Key exhange must be done before these work.
	*/
	void crypt_send(network::buffer & send_buff, const int index = 0);
	void crypt_recv(network::buffer & recv_buff, const int index = 0);

	/* Protocol for key exchange:
	Host_0 initiates outgoing connection.
	Host_1 accepts incoming connection.
	Note: All encoded numbers are big-endian.

	send_prime_and_local_result:
		Appends a random prime and local_result to the send_buff.
	recv_prime_and_remote_result:
		Sets prime and remote_result from recv_buff. Calculates local_result,
		calculates shared_secret. Appends local_result to send_buff. Returns true
		if prime is good, false if prime is bad. If prime is bad the server should
		be disconnected because key negotiation failed.
		Precondition: recv_buff.size() >= protocol::DH_KEY_SIZE * 2
		Precondition: send_buff must be empty.
		Postcondition: Host_1 is ready to crypt data.
	recv_remote_result:
		Sets remote result from recv_buff. Calculates shared_key and seeds PRNGs.
		Precondition: recv_buff.size() >= protocol::DH_KEY_SIZE
		Precondition: send_buff must be empty.
		Postcondition: Host_0 is ready to crypt data.
	*/

	//Step 1: Host_0 sends prime followed by local_result.
	void send_prime_and_local_result(network::buffer & send_buff);

	//Step 2: Host_1 receives prime and remote_result, and sends local_result.
	bool recv_prime_and_remote_result(network::buffer & recv_buff,
		network::buffer & send_buff);

	//Step 3: Host_0 receives remote_result.
	void recv_remote_result(network::buffer & recv_buff);

private:
	prime_generator & Prime_Generator;

	//Diffie-Hellman-Merkle key exchange components. (g**s % p)
	mpint g; //agreed upon base (the generator)
	mpint p; //agreed upon prime
	mpint s; //secret exponent
	mpint local_result;  //result of g^s % p with our secret s
	mpint remote_result; //result of remote host g^s % p
	mpint shared_key;    //agreed upon key (used as seed for PRNG)

	//stream cypher
	KISS PRNG_send;
	KISS PRNG_recv;
};
#endif
