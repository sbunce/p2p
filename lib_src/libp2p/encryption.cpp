#include "encryption.hpp"

encryption::encryption():
	g("2"),
	s(mpa::random(protocol_tcp::DH_key_size))
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

bool encryption::recv_p_rA(const network::buffer & buf)
{
	assert(enable_recv_p_rA);
	set_enable_false();
	enable_send_rB = true;

	assert(buf.size() == protocol_tcp::DH_key_size * 2);
	p = mpa::mpint(buf.data(), protocol_tcp::DH_key_size);
	if(!mpa::is_prime(p)){
		return false;
	}
	remote_result = mpa::mpint(buf.data() + protocol_tcp::DH_key_size, protocol_tcp::DH_key_size);
	local_result = mpa::exptmod(g, s, p);
	shared_key = mpa::exptmod(remote_result, s, p);

	std::string bin = shared_key.bin(protocol_tcp::DH_key_size);
	PRNG_send.seed(reinterpret_cast<const unsigned char *>(bin.data()), bin.size());
	PRNG_recv.seed(reinterpret_cast<const unsigned char *>(bin.data()), bin.size());
	return true;
}

void encryption::recv_rB(const network::buffer & buf)
{
	assert(enable_recv_rB);
	set_enable_false();
	enable_crypt = true;

	assert(buf.size() == protocol_tcp::DH_key_size);
	remote_result = mpa::mpint(buf.data(), protocol_tcp::DH_key_size);
	shared_key = mpa::exptmod(remote_result, s, p);

	std::string bin = shared_key.bin(protocol_tcp::DH_key_size);
	PRNG_send.seed(reinterpret_cast<const unsigned char *>(bin.data()), bin.size());
	PRNG_recv.seed(reinterpret_cast<const unsigned char *>(bin.data()), bin.size());
}

network::buffer encryption::send_p_rA()
{
	assert(enable_send_p_rA);
	set_enable_false();
	enable_recv_rB = true;

	network::buffer buf;
	p = prime_generator::singleton().random_prime();
	buf.append(p.bin(protocol_tcp::DH_key_size));
	local_result = mpa::exptmod(g, s, p);
	buf.append(local_result.bin(protocol_tcp::DH_key_size));
	return buf;
}

network::buffer encryption::send_rB()
{
	assert(enable_send_rB);
	set_enable_false();
	enable_crypt = true;

	network::buffer buf;
	buf.append(local_result.bin(protocol_tcp::DH_key_size));
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
