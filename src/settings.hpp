#ifndef H_SETTINGS
#define H_SETTINGS

//uncomment to disable all asserts, and special debug code
//#define NDEBUG

//testing features, uncomment to enable
//#define DISABLE_ENCRYPTION
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
const unsigned DOWNLOAD_RATE = std::numeric_limits<unsigned>::max(); //B/s
const unsigned UPLOAD_RATE = std::numeric_limits<unsigned>::max();   //B/s

//hard settings, not changable at runtime
const unsigned SHARE_SCAN_RATE = 2;        //share scan rate (files/second)
const unsigned PRIME_CACHE = 128;          //number of primes to keep in prime cache for diffie-hellman
const unsigned P_WAIT_TIMEOUT = 8;         //when a client download_file/download_hash_tree receives a P_WAIT it will not make any requests for this long
const unsigned P2P_PORT = 6969;            //port client connects to and server receives on
const unsigned GUI_TICK = 100;             //time(in milliseconds) between gui updates
const unsigned SPEED_AVERAGE = 8;          //how many seconds to average speed over
const unsigned RE_REQUEST = 64;            //seconds before a file block is re-requested
const unsigned RE_REQUEST_FINISHING = 16;  //seconds before a file block is re-requested when download is in finishing phase (last block requested)
const unsigned UNRESPONSIVE_TIMEOUT = 480; //if connection to server fails, new connection attempts to it are stopped for this time (seconds)

//default locations
const std::string DOWNLOAD_DIRECTORY = "download/";
const std::string SHARE_DIRECTORY = "share/";
}//end of settings namespace
#endif
