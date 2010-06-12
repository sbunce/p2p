//NOT-THREADSAFE
#ifndef H_ENCRYPTION
#define H_ENCRYPTION

//custom
#include "prime_generator.hpp"
#include "protocol_tcp.hpp"
#include "settings.hpp"

//include
#include <boost/utility.hpp>
#include <RC4.hpp>
#include <mpa.hpp>
#include <net/net.hpp>

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
	See protocol documentation for more information.
	Note: recv_p_rA returns false if invalid prime received.
	*/
	net::buffer send_p_rA();
	bool recv_p_rA(const net::buffer & buf);
	net::buffer send_rB();
	void recv_rB(const net::buffer & buf);

	/*
	Used to encrypt/decrypt buffers.
	Decrypts buffer from index to end of buffer.
	Precondition: read() = true
	*/
	void crypt_recv(net::buffer & recv_buff, const int index = 0);
	void crypt_send(net::buffer & send_buff, const int index = 0);

	//returns true when ready to encrypt/decrypt
	bool ready();

private:
	//Diffie-Hellman key exchange components. (g^s % p)
	mpa::mpint g; //agreed upon base (the generator, always 2)
	mpa::mpint p; //agreed upon prime
	mpa::mpint s; //secret exponent
	mpa::mpint local_result;  //result of g^s % p with our secret s
	mpa::mpint remote_result; //result of remote host g^s % p
	mpa::mpint shared_key;    //agreed upon key (used as seed for PRNG)

	//stream cypher
	RC4 PRNG_send;
	RC4 PRNG_recv;

	//used to assert functions not called out of order
	bool enable_send_p_rA;
	bool enable_recv_p_rA;
	bool enable_send_rB;
	bool enable_recv_rB;
	bool enable_crypt;

	//sets all of the enable_* flags to false
	void set_enable_false();
};
#endif
