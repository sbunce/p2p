#include "number_generator.hpp"

number_generator::number_generator()
{
	generate_thread = boost::thread(boost::bind(&number_generator::generate, this));
}

number_generator::~number_generator()
{
	generate_thread.interrupt();
	generate_thread.join();
	database::table::prime::write_all(Prime_Cache);
}

unsigned number_generator::prime_count()
{
	boost::mutex::scoped_lock lock(prime_mutex);
	return Prime_Cache.size();
}

mpint number_generator::random(const int bytes)
{
	unsigned char * buff = static_cast<unsigned char *>(std::malloc(bytes));
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

int number_generator::PRNG(unsigned char * buff, int size, void * data)
{
	system_urandom(buff, size);
	return size;
}

void number_generator::generate()
{
	{//begin lock scope
	boost::mutex::scoped_lock lock(prime_mutex);
	database::table::prime::read_all(Prime_Cache);
	}//end lock scope

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
			mp_prime_rabin_miller_trials(protocol::DH_KEY_SIZE * 8),
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
