#ifndef H_RANDOM
#define H_RANDOM

//include
#include <logger.hpp>

//standard
#include <fstream>

//windows PRNG
#ifdef _WIN32
	#include <wincrypt.h>
#endif

static void system_urandom(unsigned char * buff, size_t size)
{
#ifdef _WIN32
	HCRYPTPROV hProvider = 0;
	if(CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
		if(CryptGenRandom(hProvider, size, buff)){
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
	for(int x=0; x<size; ++x){
		fin.get(ch);
		*buff++ = static_cast<unsigned char>(ch);
	}
#endif
}
#endif
