#ifndef H_MESSAGE_UDP
#define H_MESSAGE_UDP

//custom
#include "protocol_udp.hpp"

//include
#include <bit_field.hpp>
#include <net/net.hpp>
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
	virtual bool expect(const net::buffer & recv_buf) = 0;
	virtual bool recv(const net::buffer & recv_buf,
		const net::endpoint & endpoint) = 0;
};

class ping : public base
{
public:
	typedef boost::function<void (const net::endpoint & endpoint,
		const net::buffer & random, const std::string & remote_ID)> handler;
	ping(handler func_in);
	virtual bool expect(const net::buffer & recv_buf);
	virtual bool recv(const net::buffer & recv_buf,
		const net::endpoint & endpoint);
private:
	handler func;
};

class pong : public base
{
public:
	typedef boost::function<void (const net::endpoint & endpoint,
		const std::string & remote_ID)> handler;
	pong(handler func_in, const net::buffer & random_in);
	virtual bool expect(const net::buffer & recv_buf);
	virtual bool recv(const net::buffer & recv_buf,
		const net::endpoint & endpoint);
private:
	handler func;
	const net::buffer random;
};

class find_node : public base
{
public:
	typedef boost::function<void (const net::endpoint & endpoint,
		const net::buffer & random, const std::string & remote_ID,
		const std::string & ID_to_find)> handler;
	find_node(handler func_in);
	virtual bool expect(const net::buffer & recv_buf);
	virtual bool recv(const net::buffer & recv_buf,
		const net::endpoint & endpoint);
private:
	handler func;
};

class host_list : public base
{
public:
	typedef boost::function<void (const net::endpoint & endpoint,
		const std::string & remote_ID,
		const std::list<std::pair<net::endpoint, unsigned char> > & hosts)> handler;
	host_list(handler func_in, const net::buffer & random_in);
	virtual bool expect(const net::buffer & recv_buf);
	virtual bool recv(const net::buffer & recv_buf,
		const net::endpoint & endpoint);
private:
	handler func;
	const net::buffer random;
};

}//end of namespace recv

namespace send{

class base
{
public:
	//contains bytes to send
	net::buffer buf;
};

class ping : public base
{
public:
	ping(const net::buffer & random, const std::string & local_ID);
};

class pong : public base
{
public:
	pong(const net::buffer & random, const std::string & local_ID);
};

class find_node : public base
{
public:
	find_node(const net::buffer & random,
		const std::string & local_ID, const std::string & ID_to_find);
};

class host_list : public base
{
public:
	host_list(const net::buffer & random, const std::string & local_ID,
		const std::list<std::pair<net::endpoint, unsigned char> > & hosts);
};

}//end of namespace send
}//end of namespace message_udp
#endif
