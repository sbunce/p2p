#include "encryption.h"

encryption::encryption()
{
	g = 2; //fast generater, supposed to be secure
	s = random_num(global::DH_KEY_SIZE*8);
	State = waiting_for_prime;
}

std::string encryption::get_prime()
{
	p = random_prime(global::DH_KEY_SIZE*8);
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

	//get Mersenne_Twister ready to create stream
	std::string seed((char *)shared_key.to_bin(), shared_key.to_bin_size());
	PRNG_send.seed(seed);
	PRNG_recv.seed(seed);

	State = ready_to_encrypt;
}

void encryption::crypt_send(std::string & bytes)
{
	assert(ready_to_encrypt);
	PRNG_send.extract_bytes(stream_send, bytes.length());
	for(int x=0; x<bytes.length(); ++x){
		bytes[x] = (unsigned char)bytes[x] ^ (unsigned char)stream_send[x];
	}
	stream_send.erase(0, bytes.length());
}

void encryption::crypt_send(char * bytes, const int & length)
{
	assert(ready_to_encrypt);
	PRNG_send.extract_bytes(stream_send, length);
	for(int x=0; x<length; ++x){
		bytes[x] = (unsigned char)bytes[x] ^ (unsigned char)stream_send[x];
	}
	stream_send.erase(0, length);
}

void encryption::crypt_recv(std::string & bytes)
{
	assert(ready_to_encrypt);
	PRNG_recv.extract_bytes(stream_recv, bytes.length());
	for(int x=0; x<bytes.length(); ++x){
		bytes[x] = (unsigned char)bytes[x] ^ (unsigned char)stream_recv[x];
	}
	stream_recv.erase(0, bytes.length());
}

void encryption::crypt_recv(char * bytes, const int & length)
{
	assert(ready_to_encrypt);
	PRNG_recv.extract_bytes(stream_recv, length);
	for(int x=0; x<length; ++x){
		bytes[x] = (unsigned char)bytes[x] ^ (unsigned char)stream_recv[x];
	}
	stream_recv.erase(0, length);
}
