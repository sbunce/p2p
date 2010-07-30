#ifndef H_K_TOKEN
#define H_K_TOKEN

//custom
#include "protocol_udp.hpp"

//include
#include <boost/optional.hpp>
#include <net/net.hpp>

//standard
#include <map>

class k_token
{
public:
	/* Tokens we issue.
	has_been_issued:
		Returns true if we have issued the specified token.
	issue:
		Issue token (the random bytes) to the specified endpoint.
	*/
	bool has_been_issued(const net::endpoint & ep, const net::buffer & random);
	void issue(const net::endpoint & ep, const net::buffer & random);

	/* Tokens issued to us.
	get_token:
		Get a token that has been issued to us.
	receive:
		Accept a token from the remote host.
	*/
	boost::optional<net::buffer> get_token(const net::endpoint & ep);
	void receive(const net::endpoint & ep, const net::buffer & random);

	/* Timed Functions
	tick:
		Called once per second to time out tokens.
	*/
	void tick();

private:
	class token
	{
	public:
		token(
			const net::buffer & random_in,
			const unsigned timeout
		);
		token(const token & T);
		//token (random bytes send in pong)
		const net::buffer random;
		//returns true if token timed out
		bool timeout();
	private:
		std::time_t time;
	};

	//keep all tokens
	std::multimap<net::endpoint, token> issued_token;

	//only keep the newest token
	std::map<net::endpoint, token> received_token;
};
#endif
