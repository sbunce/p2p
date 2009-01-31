#ifndef H_RESOLVE
#define H_RESOLVE

//boost
#include <boost/thread.hpp>

//custom
#include "global.hpp"

//networking
#ifdef WIN32
	#include <winsock.h>
#else
	#include <arpa/inet.h>
	#include <netdb.h>
#endif

namespace resolve
{
	//do not use this mutex outside the resolve namespace for any reason
	static boost::mutex gethostbyname_mutex;

	/*
	Resolve a hostname to an IP address. If an IP passed to this function nothing
	will be done.

	Returns true if resolution suceeded, or no resolution needed. False otherwise.
	*/
	static bool hostname(std::string & hostname_IP)
	{
		boost::mutex::scoped_lock lock(gethostbyname_mutex);
		hostent * he = gethostbyname(hostname_IP.c_str());
		if(he == NULL){
			LOGGER << "error resolving " << hostname_IP;
			return false;
		}
		hostname_IP = inet_ntoa(*(struct in_addr*)he->h_addr);
		return true;
	}
}
#endif
