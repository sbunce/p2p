#ifndef H_RC4
#define H_RC4

//include
#include <random.hpp>

class RC4
{
public:
	//maximum size of seed in bytes
	static const unsigned max_seed = 256;

	RC4():
		seeded(false)
	{}

	//seed from system urandom
	void seed()
	{
		unsigned char buf[max_seed];
		portable_urandom(buf, max_seed, NULL);
		seed(buf, max_seed);
	}

	void seed(unsigned char * buf, int size)
	{
		/*
		Seed must be at least one byte. It can be longer than 256 but the extra
		wouldn't be used. There is an assert for > 256 because it's wasteful.
		*/
		assert(size > 0 && size <= max_seed);
		seeded = true;

		for(i=0; i < 256; ++i){
			S[i] = i;
		}

		for(i=j=0; i < 256; ++i){
			unsigned char tmp;
			j = (j + buf[i % size] + S[i]) & 255;
			tmp = S[i];
			S[i] = S[j];
			S[j] = tmp;
		}
		i = j = 0;

		/*
		Drop 768 bytes to avoid Fluhrer, Mantin, and Shamir attack.
		http://en.wikipedia.org/wiki/Fluhrer,_Mantin,_and_Shamir_attack
		*/
		for(int x=0; x<768; ++x){
			byte();
		}
	}

	unsigned char byte()
	{
		assert(seeded);
		unsigned char temp;
		i = (i + 1) & 255;
		j = (j + S[i]) & 255;
		temp = S[i];
		S[i] = S[j];
		S[j] = temp;
		return S[(S[i] + S[j]) & 255];
	}

private:
	unsigned char S[256];
	unsigned i, j;
	bool seeded;
};
#endif
