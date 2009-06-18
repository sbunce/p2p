#include "encryption.hpp"

encryption::encryption()
{
	g = "2"; //fast generater
	s = number_generator::singleton().random(protocol::DH_KEY_SIZE);
	State = waiting_for_prime;
}

std::string encryption::get_prime()
{
	p = number_generator::singleton().random_prime();
	return std::string((char *)p.to_bin(), p.to_bin_size());
}

bool encryption::set_prime(std::string prime)
{
	assert(State == waiting_for_prime);
	p = mpint((unsigned char *)prime.data(), prime.length());

	if(p.is_prime()){
		local_result = g.exptmod(s, p);
		State = waiting_for_remote_result;
		return true;
	}else{
		std::cout << "encryption::set_prime was given invalid prime\n";
		return false;
	}
}

std::string encryption::get_local_result()
{
	return std::string((char *)local_result.to_bin(), local_result.to_bin_size());
}

void encryption::set_remote_result(std::string result)
{
	assert(State == waiting_for_remote_result);
	remote_result = mpint((unsigned char *)result.data(), result.length());

	//(remote_result)^s % p
	shared_key = remote_result.exptmod(s, p);

	//get PRNG ready to create stream
	PRNG_send.seed(shared_key.to_bin(), shared_key.to_bin_size());
	PRNG_recv.seed(shared_key.to_bin(), shared_key.to_bin_size());

	State = ready_to_encrypt;
}

void encryption::crypt_send(std::string & bytes)
{
	assert(ready_to_encrypt);
	for(int x=0; x<bytes.size(); ++x){
		bytes[x] = (unsigned char)bytes[x] ^ PRNG_send.get_byte();
	}
}

void encryption::crypt_send(char * bytes, const int & length)
{
	assert(ready_to_encrypt);
	for(int x=0; x<length; ++x){
		bytes[x] = (unsigned char)bytes[x] ^ PRNG_send.get_byte();
	}
}

void encryption::crypt_recv(std::string & bytes)
{
	assert(ready_to_encrypt);
	for(int x=0; x<bytes.size(); ++x){
		bytes[x] = (unsigned char)bytes[x] ^ PRNG_recv.get_byte();
	}
}

void encryption::crypt_recv(char * bytes, const int & length)
{
	assert(ready_to_encrypt);
	for(int x=0; x<length; ++x){
		bytes[x] = (unsigned char)bytes[x] ^ PRNG_recv.get_byte();
	}
}
