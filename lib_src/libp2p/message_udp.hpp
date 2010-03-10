#ifndef H_MESSAGE_UDP
#define H_MESSAGE_UDP

//custom
#include "protocol_udp.hpp"

//include
#include <network/network.hpp>
#include <SHA1.hpp>

namespace message_udp {
namespace recv{

class base
{
public:
	/*
	expect:
		Returns true if message expected.
	recv:
		Returns true if incoming message received. False if we don't expect the
		message. Message is removed from front of recv_buf if true returned.
	*/
	virtual bool expect(const network::buffer & recv_buf) = 0;
	virtual bool recv(network::buffer & recv_buf) = 0;
};

class ping : public base
{
public:
	typedef boost::function<void (const network::buffer & random,
		const std::string & ID)> handler;
	ping(handler func_in);
	virtual bool expect(const network::buffer & recv_buf);
	virtual bool recv(network::buffer & recv_buf);
private:
	handler func;
	const network::buffer random;
};

class pong : public base
{
public:
	typedef boost::function<void (const std::string & ID)> handler;
	pong(handler func_in, const network::buffer & random_in);
	virtual bool expect(const network::buffer & recv_buf);
	virtual bool recv(network::buffer & recv_buf);
private:
	handler func;
	const network::buffer random;
};

}//end of namespace recv

namespace send{

class base
{
public:
	//contains bytes to send
	network::buffer buf;
};

class ping : public base
{
public:
	ping(const network::buffer & random, const std::string & ID);
};

class pong : public base
{
public:
	pong(const network::buffer & random, const std::string & ID);
};

}//end of namespace send
}//end of namespace message_udp
#endif
