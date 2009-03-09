//NOT-THREADSAFE
#ifndef H_ENCRYPTION
#define H_ENCRYPTION

//boost
#include <boost/utility.hpp>

//custom
#include "CMWC4096.hpp"
#include "number_generator.hpp"
#include "protocol.hpp"
#include "settings.hpp"

//libtommath
#include <mpint.hpp>

//std
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>

class encryption : private boost::noncopyable
{
public:
	encryption();

	/*
	Protocol for key exchange:
	1. Client connects
	2. Client sends prime
	3. Client sends local_result
	4. Server sends local_result

	get_prime         - generates prime that client needs to send to server
	set_prime         - both client and server need to set the same prime
	                    returns false if prime is not prime, else true
	get_local_result  - returns g^s % p
	set_remote_result - sets the g^s % p the remote side sent
	*/
	std::string get_prime();
	bool set_prime(std::string prime);
	std::string get_local_result();
	void set_remote_result(std::string result);

	/*
	After Diffie-Hellman-Merkle these functions can be used to encrypt/decrypt.
	*/
	void crypt_send(std::string & bytes);
	void crypt_send(char * bytes, const int & length);
	void crypt_recv(std::string & bytes);
	void crypt_recv(char * bytes, const int & length);

private:
	//keeps track of encryption state to make sure no functions get called out of order
	enum state{
		waiting_for_prime,
		waiting_for_remote_result,
		ready_to_encrypt
	};
	state State;

	/*
	Information for Diffie-Hellman-Merkle key exchange.
		g^s % p
	*/
	mpint g; //agreed upon base (the generator)
	mpint p; //agreed upon prime
	mpint s; //secret exponent
	mpint local_result;  //result of doing g^s % p with our secret s
	mpint remote_result; //result of remote person doing g^s % p and sending us result
	mpint shared_key;    //agreed upon key

	//stuff for stream cypher
	CMWC4096 PRNG_send;
	CMWC4096 PRNG_recv;
	std::string stream_send; //bytes to use for stream cypher
	std::string stream_recv; //bytes to use for stream cypher
};
#endif
