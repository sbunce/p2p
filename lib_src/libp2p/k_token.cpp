#include "k_token.hpp"

//BEGIN token
k_token::token::token(
	const net::buffer & random_in,
	const unsigned timeout
):
	random(random_in),
	time(std::time(NULL) + timeout)
{

}

k_token::token::token(const token & T):
	random(T.random),
	time(T.time)
{

}

bool k_token::token::timeout()
{
	return std::time(NULL) > time;
}
//END token

boost::optional<net::buffer> k_token::get_token(const net::endpoint & ep)
{
	std::map<net::endpoint, token>::iterator it = received_token.find(ep);
	if(it != received_token.end()){
		return it->second.random;
	}
	return boost::optional<net::buffer>();
}

bool k_token::has_been_issued(const net::endpoint & ep,
	const net::buffer & random)
{
	typedef std::multimap<net::endpoint, token>::iterator it_t;
	std::pair<it_t, it_t> p = issued_token.equal_range(ep);
	for(; p.first != p.second; ++p.first){
		if(p.first->second.random == random){
			return true;
		}
	}
	return false;
}

void k_token::issue(const net::endpoint & ep, const net::buffer & random)
{
	issued_token.insert(std::make_pair(ep, token(random,
		protocol_udp::store_token_issued_timeout)));
}

void k_token::receive(const net::endpoint & ep, const net::buffer & random)
{
	received_token.erase(ep);
	received_token.insert(std::make_pair(ep, token(random,
		protocol_udp::store_token_received_timeout)));
}

void k_token::tick()
{
	for(std::multimap<net::endpoint, token>::iterator it_cur = issued_token.begin();
		it_cur != issued_token.end();)
	{
		if(it_cur->second.timeout()){
			issued_token.erase(it_cur++);
		}else{
			++it_cur;
		}
	}
	for(std::map<net::endpoint, token>::iterator it_cur = received_token.begin();
		it_cur != received_token.end();)
	{
		if(it_cur->second.timeout()){
			received_token.erase(it_cur++);
		}else{
			++it_cur;
		}
	}
}
