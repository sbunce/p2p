#ifndef H_RANDOM
#define H_RANDOM

//include
#include <logger.hpp>
#include <network/network.hpp>

//standard
#include <fstream>

//windows PRNG
#ifdef _WIN32
	/*
	Even though this header is not relevant here it must be included before
	wincrypt.h otherwise there will be errors.
	*/
	#include <ws2tcpip.h>
	#include <wincrypt.h>
#endif

//tommath requires this function signature
static int portable_urandom(unsigned char * buf, int size, void * data = NULL)
{
#ifdef _WIN32
	HCRYPTPROV hProvider = 0;
	if(CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
		if(CryptGenRandom(hProvider, size, buf)){
			CryptReleaseContext(hProvider, 0); 
		}else{
			LOGGER(logger::fatal) << "error generating random number";
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

static network::buffer portable_urandom(const int size)
{
	network::buffer buf;
	buf.resize(size);
	portable_urandom(buf.data(), size, NULL);
	return buf;
}
#endif
