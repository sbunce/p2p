#include "encryption.hpp"

encryption::encryption(prime_generator & Prime_Generator_in):
	Prime_Generator(Prime_Generator_in),
	g("2"),
	s(mpint::random(protocol::DH_KEY_SIZE, &portable_urandom))
{

}

void encryption::crypt_send(network::buffer & send_buff, const int index)
{
	for(int x=index; x<send_buff.size(); ++x){
		send_buff[x] ^= PRNG_send.byte();
	}
}
void encryption::crypt_recv(network::buffer & recv_buff, const int index)
{
	for(int x=index; x<recv_buff.size(); ++x){
		recv_buff[x] ^= PRNG_recv.byte();
	}
}

bool encryption::recv_prime_and_remote_result(network::buffer & recv_buff,
	network::buffer & send_buff)
{
	assert(recv_buff.size() >= protocol::DH_KEY_SIZE * 2);
	assert(send_buff.empty());

	//recv prime
	p = mpint(recv_buff.data(), protocol::DH_KEY_SIZE);
	if(!p.is_prime()){
		//invalid prime
		return false;
	}
	recv_buff.erase(0, protocol::DH_KEY_SIZE);

	//recv remote_result
	remote_result = mpint(recv_buff.data(), protocol::DH_KEY_SIZE);
	recv_buff.erase(0, protocol::DH_KEY_SIZE);

	//calculate local_result
	local_result = g.exptmod(s, p);

	//calculate shared_key
	shared_key = remote_result.exptmod(s, p);

	//seed PRNGS
	PRNG_send.seed(shared_key.to_bin(), shared_key.to_bin_size());
	PRNG_recv.seed(shared_key.to_bin(), shared_key.to_bin_size());

	//send local_result
	send_buff.append(local_result.to_bin(), local_result.to_bin_size());

	return true;
}

void encryption::recv_remote_result(network::buffer & recv_buff)
{
	assert(recv_buff.size() >= protocol::DH_KEY_SIZE);

	//recv remote_result
	remote_result = mpint(recv_buff.data(), protocol::DH_KEY_SIZE);
	recv_buff.erase(0, protocol::DH_KEY_SIZE);

	//calculate shared_key
	shared_key = remote_result.exptmod(s, p);

	//seed PRNGs
	PRNG_send.seed(shared_key.to_bin(), shared_key.to_bin_size());
	PRNG_recv.seed(shared_key.to_bin(), shared_key.to_bin_size());
}

void encryption::send_prime_and_local_result(network::buffer & send_buff)
{
	p = Prime_Generator.random_prime();
	send_buff.append(p.to_bin(), p.to_bin_size());
	local_result = g.exptmod(s, p);
	send_buff.append(local_result.to_bin(), local_result.to_bin_size());
}
