#ifndef H_SETTINGS
#define H_SETTINGS

//standard
#include <limits>
#include <string>

namespace settings
{
//default settings, may be changed at runtime
const unsigned MAX_CONNECTIONS = 1024;
const unsigned MAX_DOWNLOAD_RATE = 0; //no limit
const unsigned MAX_UPLOAD_RATE = 0;   //no limit

//hard settings, not changable at runtime
const int PRIME_CACHE = 32;         //number of primes to keep in prime cache
const int DATABASE_POOL_SIZE = 8;   //size of database connection pool
const int SHARE_BUFFER_SIZE = 1024; //size of buffers between share pipeline stages
}//end of namespace settings
#endif
