#ifndef H_PORTABLE_RANDOM
#define H_PORTABLE_RANDOM

//include
#include <boost/cstdint.hpp>
#include <logger.hpp>
#include <net/net.hpp>

//standard
#include <fstream>

//windows PRNG
#ifdef _WIN32
	/*
	Even though this header is not relevant here it must be included before
	wincrypt.h otherwise there will be errors.
	*/
	//#include <ws2tcpip.h>
	#include <wincrypt.h>
#endif

namespace{
namespace portable{

//tommath requires this function signature
int urandom(unsigned char * buf, int size, void * data = NULL)
{
	assert(buf);
	assert(size >= 0);
#ifdef _WIN32
	HCRYPTPROV hProvider = 0;
	if(CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
		if(CryptGenRandom(hProvider, size, buf)){
			CryptReleaseContext(hProvider, 0); 
		}else{
			LOG << "error generating random number";
			exit(1);
		}
	}
#else
	std::fstream fin("/dev/urandom", std::ios::in | std::ios::binary);
	assert(fin.is_open());
	char ch;
	for(int x=0; x<size; ++x){
		fin.get(ch);
		*buf++ = static_cast<unsigned char>(ch);
	}
#endif
	return size;
}

//returns random bytes of specified size
net::buffer urandom(const int size)
{
	assert(size >= 0);
	net::buffer buf;
	buf.resize(size);
	urandom(buf.data(), size);
	return buf;
}

boost::uint64_t urandom()
{
	union{
		boost::uint64_t n;
		unsigned char b[sizeof(boost::uint64_t)];
	}tmp;
	urandom(tmp.b, sizeof(boost::uint64_t));
	return tmp.n;
}

//returns number between beg, and end - 1
boost::uint64_t roll(const boost::uint64_t beg, const boost::uint64_t end)
{
	assert(beg < end);
	assert(beg + end > beg);
	return (urandom() % (end - beg)) + beg;
}

//returns number between 0 and end - 1
boost::uint64_t roll(const boost::uint64_t end)
{
	return roll(0, end);
}

}//end of namespace random
}//end of unnamed namespace
#endif
