#ifndef H_NET_INIT
#define H_NET_INIT

//custom
#include "system_include.hpp"

//include
#include <logger.hpp>

namespace net{
class init
{
public:
	/*
	The object calls start() in ctor and stop in dtor. It can be used this way
	or the start() and stop() functions can be called.
	*/
	init();
	~init();

	/*
	For every call to start() there must be exactly one call to stop().
	*/
	static void start();
	static void stop();
};
}//end namespace net
#endif
