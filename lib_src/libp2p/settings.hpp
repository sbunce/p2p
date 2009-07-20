#ifndef H_SETTINGS
#define H_SETTINGS

//standard
#include <limits>
#include <string>

//windows.h defines max that interferes with std::numeric_limits<int>::max()
#ifdef _WIN32
	#ifdef max
		#undef max
	#endif
#endif

namespace settings
{
//soft settings, may be changed at runtime
const unsigned MAX_CONNECTIONS = 1024;
const unsigned MAX_DOWNLOAD_RATE = std::numeric_limits<unsigned>::max(); //B/s
const unsigned MAX_UPLOAD_RATE = std::numeric_limits<unsigned>::max();   //B/s

//hard settings, not changable at runtime
const int SHARE_SCAN_RATE = 100;  //share scan rate (files/second)
const int PRIME_CACHE = 512;      //number of primes to keep in prime cache
const int P2P_PORT = 6969;        //port client connects to and server receives on
const int DATABASE_POOL_SIZE = 8; //size of database connection pool
const int THREAD_POOL_SIZE = 2;   //size of thread pool
}//end of namespace settings
#endif
