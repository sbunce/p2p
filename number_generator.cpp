#include "number_generator.hpp"

number_generator::number_generator()
{
	genprime_thread = boost::thread(boost::bind(&number_generator::genprime_loop, this));
}

number_generator::~number_generator()
{
	genprime_thread.interrupt();
	genprime_thread.join();
}

unsigned number_generator::prime_count()
{
	return DB_Prime.count();
}

mpint number_generator::random_mpint(const int & bytes)
{
	unsigned char buff[protocol::DH_KEY_SIZE];
	PRNG(buff, bytes, NULL);
	return mpint(buff, bytes);
}

mpint number_generator::random_prime_mpint()
{
	try{
		mpint random;
		random_prime_mpint_mutex.lock();
		while(!DB_Prime.retrieve(random)){
			random_prime_mpint_cond.wait(random_prime_mpint_mutex);
		}
		random_prime_mpint_mutex.unlock();
		genprime_loop_cond.notify_one();
		return random;
	}catch(const boost::thread_interrupted & ex){
		random_prime_mpint_mutex.unlock();
		LOGGER << "thread should not be interrupted";
		exit(1);
	}
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

void number_generator::genprime_loop()
{
	try{
		while(true){
			boost::this_thread::interruption_point();
			genprime_loop_mutex.lock();
			while(DB_Prime.count() >= settings::PRIME_CACHE){
				genprime_loop_cond.wait(genprime_loop_mutex);
			}
			genprime_loop_mutex.unlock();
			mpint random;
			mp_prime_random_ex(
				&random.c_struct(),
				1,                       //Miller-Rabin tests
				protocol::DH_KEY_SIZE*8, //size (bits) of prime to generate
				0,                       //optional flags
				&PRNG,
				NULL                     //optional void* that can be passed to PRNG
			);
			DB_Prime.add(random);
			random_prime_mpint_cond.notify_one();
		}
	}catch(const boost::thread_interrupted & ex){
		genprime_loop_mutex.unlock();
	}
}
