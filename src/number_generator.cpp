#include "number_generator.hpp"

//BEGIN STATIC
void number_generator::init()
{
	Number_Generator();
}

unsigned number_generator::prime_count()
{
	return Number_Generator().prime_count_priv();
}

mpint number_generator::random_mpint(const int & bytes)
{
	return Number_Generator().random_mpint_priv(bytes);
}

mpint number_generator::random_prime_mpint()
{
	return Number_Generator().random_prime_mpint_priv();
}

number_generator & number_generator::Number_Generator()
{
	static boost::mutex Mutex;
	boost::mutex::scoped_lock lock(Mutex);
	static number_generator N;
	return N;
}

int number_generator::PRNG(unsigned char * buff, int length, void * data)
{
#ifdef WIN32
	HCRYPTPROV hProvider = 0;
	if(CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
		if(CryptGenRandom(hProvider, sizeof(buff), buff)){
			CryptReleaseContext(hProvider, 0); 
			return length;
		}
	}
	LOGGER << "error generating random number";
	exit(1);
#else
	char ch;
	int length_start = length;
	std::fstream fin("/dev/urandom", std::ios::in | std::ios::binary);
	while(length--){
		fin.get(ch);
		*buff++ = (unsigned char)ch;
	}
	return length_start;
#endif
}
//END STATIC

number_generator::number_generator()
{
	genprime_thread = boost::thread(boost::bind(&number_generator::genprime_loop, this));
}

number_generator::~number_generator()
{
	genprime_thread.interrupt();
	genprime_thread.join();
}

unsigned number_generator::prime_count_priv()
{
	return DB_Prime.count();
}

mpint number_generator::random_mpint_priv(const int & bytes)
{
	assert(bytes <= global::DH_KEY_SIZE);
	unsigned char buff[global::DH_KEY_SIZE];
	PRNG(buff, bytes, NULL);
	return mpint(buff, bytes);
}

mpint number_generator::random_prime_mpint_priv()
{
	mpint random;
	while(!DB_Prime.retrieve(random)){
		//no prime available, wait for one
		portable_sleep::yield();
	}
	return random;
}

void number_generator::genprime_loop()
{
	while(true){
		boost::this_thread::interruption_point();
		if(DB_Prime.count() >= global::PRIME_CACHE){
			//enough primes generated, sleep for a while
			portable_sleep::ms(1000);
			continue;
		}

		mpint random;
		mp_prime_random_ex(
			&random.c_struct(),
			1,                     //Miller-Rabin tests
			global::DH_KEY_SIZE*8, //size (bits) of prime to generate
			0,                     //optional flags
			&PRNG,
			NULL                   //optional void* that can be passed to PRNG
		);
		DB_Prime.add(random);
	}
}
