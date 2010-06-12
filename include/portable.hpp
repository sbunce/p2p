#ifndef H_PORTABLE
#define H_PORTABLE

//include
#include <boost/cstdint.hpp>
#include <logger.hpp>
#include <net/net.hpp>

//system specific
#ifdef _WIN32
	#include <process.h>
	#include <wincrypt.h>
#else
	#include <sys/types.h> 
	#include <unistd.h>
#endif

//standard
#include <fstream>

/*
This namespace contains functions which wrap similar systems specific functions.
*/
namespace{
namespace portable{

int getpid()
{
	#ifdef _WIN32
	return _getpid();
	#else
	return ::getpid();
	#endif
}

#ifdef _WIN32
	#define gid_t int
#endif
int setgid(gid_t gid)
{
	#ifdef _WIN32
	//not supported on windows, always return success
	return 0;
	#else
	return ::setgid(gid);
	#endif
}

#ifdef _WIN32
	#define uid_t int
#endif
int setuid(uid_t uid)
{
	#ifdef _WIN32
	//not supported on windows, always return success
	return 0;
	#else
	return ::setuid(uid);
	#endif
}

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

//returns random boost::uint64_t
boost::uint64_t urandom()
{
	union{
		boost::uint64_t n;
		unsigned char b[sizeof(boost::uint64_t)];
	}tmp;
	urandom(tmp.b, sizeof(boost::uint64_t));
	return tmp.n;
}

//returns random number between beg, and end - 1
boost::uint64_t urandom_roll(const boost::uint64_t beg, const boost::uint64_t end)
{
	assert(beg < end);
	assert(beg + end > beg);
	return (urandom() % (end - beg)) + beg;
}

//returns random number between 0 and end - 1
boost::uint64_t urandom_roll(const boost::uint64_t end)
{
	return urandom_roll(0, end);
}

}//end namespace portable
}//end unnamed namespace
#endif
