#ifndef H_SETTINGS
#define H_SETTINGS

//uncomment to disable all asserts, and special debug code
//#define NDEBUG

//testing features, uncomment to enable
//#define CORRUPT_HASH_BLOCK_TEST
//#define CORRUPT_FILE_BLOCK_TEST

//std
#include <limits>
#include <string>

#ifdef WIN32
#include <windows.h>
#undef max //this interferes with std::numeric_limits<int>::max()
#endif

namespace settings
{
const std::string NAME = "p2p";
const std::string VERSION = "0.00 pre-alpha";

//soft settings, may be changed at runtime
const unsigned MAX_CONNECTIONS = 1024;
const unsigned MAX_DOWNLOAD_RATE = std::numeric_limits<unsigned>::max(); //B/s
const unsigned MAX_UPLOAD_RATE = std::numeric_limits<unsigned>::max();   //B/s

//hard settings, not changable at runtime
const unsigned SHARE_SCAN_RATE = 10;      //share scan rate (files/second)
const unsigned PRIME_CACHE = 128;         //number of primes to keep in prime cache
const unsigned P_WAIT_TIMEOUT = 8;        //how long to wait after receiving P_WAIT
const unsigned P2P_PORT = 6969;           //port client connects to and server receives on
const unsigned GUI_TICK = 100;            //time(milliseconds) between gui updates
const unsigned RE_REQUEST = 64;           //how long to wait for blocks
const unsigned RE_REQUEST_FINISHING = 16; //when finishing file how long to wait for blocks
}//end of settings namespace
#endif
