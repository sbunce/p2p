#include "number_generator.hpp"

number_generator::number_generator()
{
	generate_thread = boost::thread(boost::bind(&number_generator::generate, this));
}

unsigned number_generator::prime_count()
{
	boost::mutex::scoped_lock lock(prime_mutex);
	return Prime_Cache.size();
}

mpint number_generator::random(const int & bytes)
{
	unsigned char * buff = (unsigned char *)std::malloc(bytes);
	assert(buff);
	PRNG(buff, bytes, NULL);
	mpint temp(buff, bytes);
	std::free(buff);
	return temp;
}

mpint number_generator::random_prime()
{
	mpint temp;
	boost::mutex::scoped_lock lock(prime_mutex);
	while(Prime_Cache.empty()){
		prime_remove_cond.wait(prime_mutex);
	}
	temp = Prime_Cache.back();
	Prime_Cache.pop_back();
	prime_generate_cond.notify_one();
	return temp;
}

int number_generator::PRNG(unsigned char * buff, int length, void * data)
{
#ifdef WIN32
	HCRYPTPROV hProvider = 0;
	if(CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
		if(CryptGenRandom(hProvider, length, buff)){
			CryptReleaseContext(hProvider, 0); 
			return length;
		}
	}
	LOGGER << "error generating random number";
	exit(1);
#else
	std::fstream fin("/dev/urandom", std::ios::in | std::ios::binary);
	char ch;
	for(int x=0; x<length; ++x){
		fin.get(ch);
		*buff++ = (unsigned char)ch;
	}
	return length;
#endif
}

void number_generator::generate()
{
	{ //DB will close when it leaves this scope
	boost::mutex::scoped_lock lock(prime_mutex);
	database::connection DB(path::database());
	database::table::prime::read_all(Prime_Cache, DB);
	}

	mpint random;
	while(true){
		boost::this_thread::interruption_point();
		{//begin lock scope
		boost::mutex::scoped_lock lock(prime_mutex);
		while(Prime_Cache.size() >= settings::PRIME_CACHE){
			prime_generate_cond.wait(prime_mutex);
		}
		}//end lock scope

		//this should not be locked
		mp_prime_random_ex(
			&random.c_struct(),
			10,                        //Miller-Rabin tests
			protocol::DH_KEY_SIZE * 8, //size (bits) of prime to generate
			0,                         //optional flags
			&PRNG,                     //random byte source
			NULL                       //optional void * passed to PRNG
		);

		{//begin lock scope
		boost::mutex::scoped_lock lock(prime_mutex);
		Prime_Cache.push_back(random);
		}//end lock scope

		//notify possible threads waiting for prime to be generated
		prime_remove_cond.notify_all();
	}
}

void number_generator::stop()
{
	generate_thread.interrupt();
	generate_thread.join();
	database::connection DB(path::database());
	database::table::prime::write_all(Prime_Cache, DB);
}
