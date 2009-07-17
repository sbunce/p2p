//NOT-THREADSAFE
#ifndef H_ENCRYPTION
#define H_ENCRYPTION

//custom
#include "number_generator.hpp"
#include "protocol.hpp"
#include "settings.hpp"

//include
#include <boost/utility.hpp>
#include <RC4.hpp>
#include <tommath/mpint.hpp>

//std
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>

class encryption : private boost::noncopyable
{
public:
	encryption();

	/* Protocol for key exchange:
	1. Client connects
	2. Client sends prime and local_result
	4. Server sends local_result

	get_prime:
		Generates prime that client needs to send to server.
	set_prime:
		Sets p. Both client and server need to set the same p. Returns false if
	   passed in value not prime.
	get_local_result:
		Returns g^s % p.
	set_remote_result:
		Sets the g^s % p the remote side sent.
	*/
	std::string get_prime();
	bool set_prime(std::string prime);
	std::string get_local_result();
	void set_remote_result(std::string result);

	/*
	After Diffie-Hellman-Merkle these functions can be used to encrypt/decrypt.
	T must be string-like. It must have .size() and [] must return bytes.
	*/
	template<typename T>
	void crypt_send(T & buff)
	{
		assert(ready_to_encrypt);
		for(int x=0; x<buff.size(); ++x){
			buff[x] = static_cast<unsigned char>(buff[x]) ^ PRNG_send.get_byte();
		}
	}

	template<typename T>
	void crypt_recv(T & buff)
	{
		assert(ready_to_encrypt);
		for(int x=0; x<buff.size(); ++x){
			buff[x] = static_cast<unsigned char>(buff[x]) ^ PRNG_recv.get_byte();
		}
	}

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

	//stream cypher
	RC4 PRNG_send;
	RC4 PRNG_recv;
};
#endif
