#ifndef H_NETWORK_SOCKET_BASE
#define H_NETWORK_SOCKET_BASE

//custom
#include "start.hpp"

class socket_base
{
	network::start Start;
public:
	socket_base(){}
	virtual ~socket_base(){}

	/*

	*/
	virtual void close();
	virtual void is_open();
	virtual void open(const network::endpoint & E) = 0;

protected:
	int socket_FD;
};
#endif
