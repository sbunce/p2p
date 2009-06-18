#ifndef H_RANDOM
#define H_RANDOM

//include
#include <logger.hpp>

//std
#include <fstream>

//windows PRNG
#ifdef WIN32
#include <wincrypt.h>
#endif

namespace custom {

/*
Generates random bytes in a system specific way. This is good enough to use as
a CSPRNG but it's slow. It's adviseable to use this to get a seed for a faster
PRNG.
*/
static void random(unsigned char * buff, size_t length)
{
#ifdef WIN32
	HCRYPTPROV hProvider = 0;
	if(CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
		if(CryptGenRandom(hProvider, length, buff)){
			CryptReleaseContext(hProvider, 0); 
		}else{
			LOGGER << "error generating random number";
			exit(1);
		}
	}
#else
	std::fstream fin("/dev/urandom", std::ios::in | std::ios::binary);
	assert(fin.is_open());
	char ch;
	for(int x=0; x<length; ++x){
		fin.get(ch);
		*buff++ = static_cast<unsigned char>(ch);
	}
#endif
}

}//end namespace custom
#endif
