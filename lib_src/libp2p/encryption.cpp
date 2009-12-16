#include "encryption.hpp"

encryption::encryption():
	g("2"),
	s(mpint::random(protocol::DH_KEY_SIZE, &portable_urandom))
{
	set_enable_false();
	enable_send_p_rA = true;
	enable_recv_p_rA = true;
}

void encryption::crypt_recv(network::buffer & recv_buff, const int index)
{
	assert(enable_crypt);
	for(int x=index; x<recv_buff.size(); ++x){
		recv_buff[x] ^= PRNG_recv.byte();
	}
}

void encryption::crypt_send(network::buffer & send_buff, const int index)
{
	assert(enable_crypt);
	for(int x=index; x<send_buff.size(); ++x){
		send_buff[x] ^= PRNG_send.byte();
	}
}

bool encryption::ready()
{
	return enable_crypt;
}

bool encryption::recv_p_rA(network::buffer & buf)
{
	assert(enable_recv_p_rA);
	set_enable_false();
	enable_send_rB = true;

	assert(buf.size() == protocol::DH_KEY_SIZE * 2);
	p = mpint(buf.data(), protocol::DH_KEY_SIZE);
	if(!p.is_prime()){
		//invalid prime
		return false;
	}
	remote_result = mpint(buf.data() + protocol::DH_KEY_SIZE, protocol::DH_KEY_SIZE);
	local_result = g.exptmod(s, p);
	shared_key = remote_result.exptmod(s, p);
	if(shared_key.to_bin_size() == protocol::DH_KEY_SIZE){
		PRNG_send.seed(shared_key.to_bin(), shared_key.to_bin_size());
		PRNG_recv.seed(shared_key.to_bin(), shared_key.to_bin_size());
	}else{
		//high order byte(s) empty, pad big-endian representation of mpint
		unsigned char tmp[protocol::DH_KEY_SIZE];
		std::memset(tmp, 0, protocol::DH_KEY_SIZE);
		std::memcpy(tmp + (protocol::DH_KEY_SIZE - shared_key.to_bin_size()),
			shared_key.to_bin(), shared_key.to_bin_size());
		PRNG_send.seed(tmp, protocol::DH_KEY_SIZE);
		PRNG_recv.seed(tmp, protocol::DH_KEY_SIZE);
	}
	return true;
}

void encryption::recv_rB(network::buffer & buf)
{
	assert(enable_recv_rB);
	set_enable_false();
	enable_crypt = true;

	assert(buf.size() == protocol::DH_KEY_SIZE);
	remote_result = mpint(buf.data(), protocol::DH_KEY_SIZE);
	shared_key = remote_result.exptmod(s, p);
	if(shared_key.to_bin_size() == protocol::DH_KEY_SIZE){
		PRNG_send.seed(shared_key.to_bin(), shared_key.to_bin_size());
		PRNG_recv.seed(shared_key.to_bin(), shared_key.to_bin_size());
	}else{
		//high order byte(s) empty, pad big-endian representation of mpint
		unsigned char tmp[protocol::DH_KEY_SIZE];
		std::memset(tmp, 0, protocol::DH_KEY_SIZE);
		std::memcpy(tmp + (protocol::DH_KEY_SIZE - shared_key.to_bin_size()),
			shared_key.to_bin(), shared_key.to_bin_size());
		PRNG_send.seed(tmp, protocol::DH_KEY_SIZE);
		PRNG_recv.seed(tmp, protocol::DH_KEY_SIZE);
	}
}

network::buffer encryption::send_p_rA()
{
	assert(enable_send_p_rA);
	set_enable_false();
	enable_recv_rB = true;

	network::buffer buf;
	p = prime_generator::singleton().random_prime();
	if(p.to_bin_size() == protocol::DH_KEY_SIZE){
		buf.append(p.to_bin(), p.to_bin_size());
	}else{
		//high order byte(s) empty, pad big-endian representation of mpint
		unsigned char tmp[protocol::DH_KEY_SIZE];
		std::memset(tmp, 0, protocol::DH_KEY_SIZE);
		std::memcpy(tmp + (protocol::DH_KEY_SIZE - p.to_bin_size()), p.to_bin(),
			p.to_bin_size());
		buf.append(tmp, protocol::DH_KEY_SIZE);
	}
	local_result = g.exptmod(s, p);
	if(local_result.to_bin_size() == protocol::DH_KEY_SIZE){
		buf.append(local_result.to_bin(), local_result.to_bin_size());
	}else{
		//high order byte(s) empty, pad big-endian representation of mpint
		unsigned char tmp[protocol::DH_KEY_SIZE];
		std::memset(tmp, 0, protocol::DH_KEY_SIZE);
		std::memcpy(tmp + (protocol::DH_KEY_SIZE - local_result.to_bin_size()),
			local_result.to_bin(), local_result.to_bin_size());
		buf.append(tmp, protocol::DH_KEY_SIZE);
	}
	return buf;
}

network::buffer encryption::send_rB()
{
	assert(enable_send_rB);
	set_enable_false();
	enable_crypt = true;

	network::buffer buf;
	if(local_result.to_bin_size() == protocol::DH_KEY_SIZE){
		buf.append(local_result.to_bin(), local_result.to_bin_size());
	}else{
		//high order byte(s) empty, pad big-endian representation of mpint
		unsigned char tmp[protocol::DH_KEY_SIZE];
		std::memset(tmp, 0, protocol::DH_KEY_SIZE);
		std::memcpy(tmp + (protocol::DH_KEY_SIZE - local_result.to_bin_size()),
			local_result.to_bin(), local_result.to_bin_size());
		buf.append(tmp, protocol::DH_KEY_SIZE);
	}
	return buf;
}

void encryption::set_enable_false()
{
	enable_send_p_rA = false;
	enable_recv_p_rA = false;
	enable_send_rB = false;
	enable_recv_rB = false;
	enable_crypt = false;
}
