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
	Mersenne_Twister_send.seed(seed);
	Mersenne_Twister_recv.seed(seed);

	State = ready_to_encrypt;
}

void encryption::grow_stream(const int & bytes, mersenne_twister & MT, std::string & stream)
{
	if(bytes > stream.size()){
		//stream not big enough to encrypt bytes, grow stream
		int bytes_needed = bytes - stream.size();
		int iterations = bytes_needed / sha::HASH_LENGTH + 1;
		std::string bytes;
		for(int x=0; x<iterations; ++x){
			//get 64 bytes to feed to SHA
			for(int x=0; x<16; ++x){
				bytes += MT.extract_4_bytes();
			}
			SHA.init();
			SHA.load(bytes.c_str(), bytes.length());
			SHA.end();
			stream.append(SHA.raw_hash(), sha::HASH_LENGTH);
			bytes.clear();
		}
	}
}

void encryption::crypt_send(std::string & bytes)
{
	assert(ready_to_encrypt);
	grow_stream(bytes.length(), Mersenne_Twister_send, stream_send);
	for(int x=0; x<bytes.length(); ++x){
		bytes[x] = (unsigned char)bytes[x] ^ (unsigned char)stream_send[x];
	}
	stream_send.erase(0, bytes.length());
}

void encryption::crypt_send(char * bytes, const int & length)
{
	assert(ready_to_encrypt);
	grow_stream(length, Mersenne_Twister_send, stream_send);
	for(int x=0; x<length; ++x){
		bytes[x] = (unsigned char)bytes[x] ^ (unsigned char)stream_send[x];
	}
	stream_send.erase(0, length);
}

void encryption::crypt_recv(std::string & bytes)
{
	assert(ready_to_encrypt);
	grow_stream(bytes.length(), Mersenne_Twister_recv, stream_recv);
	for(int x=0; x<bytes.length(); ++x){
		bytes[x] = (unsigned char)bytes[x] ^ (unsigned char)stream_recv[x];
	}
	stream_recv.erase(0, bytes.length());
}

void encryption::crypt_recv(char * bytes, const int & length)
{
	assert(ready_to_encrypt);
	grow_stream(length, Mersenne_Twister_recv, stream_recv);
	for(int x=0; x<length; ++x){
		bytes[x] = (unsigned char)bytes[x] ^ (unsigned char)stream_recv[x];
	}
	stream_recv.erase(0, length);
}
