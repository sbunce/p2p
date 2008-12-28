#ifndef H_GLOBAL
#define H_GLOBAL

//uncomment to disable all asserts
//#define NDEBUG

//testing features, uncomment to enable
//#define CORRUPT_HASH_BLOCK_TEST
//#define CORRUPT_FILE_BLOCK_TEST
#define ALLOW_LOCALHOST_CONNECTION

/*
This stops compilation when boost is too old. The MIN_MAJOR_VERSION and
MIN_MINOR_VERSION need to be set to the minimum allowed version of boost.

example: 1.36 would have major 1 and minor 36
*/
#include <boost/version.hpp>
#define MIN_MAJOR_VERSION 1
#define MIN_MINOR_VERSION 36

#define CURRENT_MAJOR_VERSION BOOST_VERSION / 100000
#define CURRENT_MINOR_VERSION BOOST_VERSION / 100 % 1000

#if CURRENT_MAJOR_VERSION < MIN_MAJOR_VERSION
	#error boost version too old
#elif CURRENT_MAJOR_VERSION == MIN_MAJOR_VERSION
	#if CURRENT_MINOR_VERSION < MIN_MINOR_VERSION
		#error boost version too old
	#endif
#endif

//boost
#include <boost/cstdint.hpp>

//custom
#include "logger.h"
#include "sha.h"

//std
#include <cassert>
#include <limits>
#include <string>

#ifdef WIN32
#include <windows.h>
#undef max //this interferes with numeric_limits::max
#endif

//replacement for system specific sleep functions
namespace portable_sleep
{
	//sleep for specified number of milliseconds
	inline void ms(const unsigned & milliseconds)
	{
		//if milliseconds = 0 then yield should be used
		assert(milliseconds != 0);
		#ifdef WIN32
		Sleep(milliseconds);
		#else
		usleep(milliseconds * 1000);
		#endif
	}

	//yield time slice
	inline void yield()
	{
		#ifdef WIN32
		Sleep(0);
		#else
		usleep(1);
		#endif
	}
}

namespace global
{
	const std::string NAME = "p2p";
	const std::string VERSION = "0.00 pre-alpha";

	//default settings, may be changed at run time
	const unsigned MAX_CONNECTIONS = 1024; //maximum number of connections the server will accept
	const unsigned UP_SPEED = std::numeric_limits<unsigned>::max();   //upload speed limit (B/s)
	const unsigned DOWN_SPEED = std::numeric_limits<unsigned>::max(); //download speed limit (B/s)

	//hard settings, not changed at runtime
	const unsigned DB_TIMEOUT = 1000;         //database timeout (ms)
	const unsigned PRIME_CACHE = 256;         //number of primes to keep in prime cache for diffie-hellman
	const unsigned DH_KEY_SIZE = 64;          //size of key exchanged with diffie-hellman (bytes)
	const unsigned PIPELINE_SIZE = 16;        //max pre-requests that can be done (must be >= 2)
	const unsigned RE_REQUEST = 16;           //seconds before a file block is re-requested
	const unsigned P_WAIT_TIMEOUT = 8;        //when a client download_file/download_hash_tree receives a P_WAIT it will not make any requests for this long
	const unsigned RE_REQUEST_FINISHING = 4;  //seconds before a file block is re-requested when download is in finishing phase (last block requested)
	const unsigned P2P_PORT = 6969;           //port client connects to and server receives on
	const unsigned GUI_TICK = 100;            //time(in milliseconds) between gui updates
	const unsigned SPEED_AVERAGE = 4;         //how many seconds to average speed over
	const unsigned TIMEOUT = 16;              //how long before an unresponsive socket times out
	const unsigned UNRESPONSIVE_TIMEOUT = 60; //if connection to server fails, new connection attempts to it are stopped for this time (seconds)

	//default locations
	const std::string DOWNLOAD_DIRECTORY = "download/";
	const std::string SHARE_DIRECTORY = "share/";
	const std::string HASH_DIRECTORY = "hash/";
	const std::string DATABASE_PATH = "database";

	/*
	The size of the file block is always related to the size of a hash block so
	that the same command and function can be used to serve both.

	HASH_BLOCK_SIZE - number of hashes in a child node in the hash tree.
		Note: HASH_BLOCK_SIZE % 2 must = 0 and should be >= 2;
	FILE_BLOCK_SIZE - number of bytes in a file block.
	*/
	const unsigned HASH_BLOCK_SIZE = 256;
	const unsigned FILE_BLOCK_SIZE = HASH_BLOCK_SIZE * sha::HASH_SIZE;

	//protocol commands client <-> server
	const char P_ERROR = (char)0;
	const unsigned P_ERROR_SIZE = 1;
	const char P_BLOCK = (char)5;
	const unsigned P_BLOCK_TO_CLIENT_SIZE = FILE_BLOCK_SIZE + 1;
	const unsigned P_BLOCK_TO_SERVER_SIZE = 10;

	//protocol commands client -> server
	const char P_REQUEST_SLOT_HASH = (char)1;
	const unsigned P_REQUEST_SLOT_HASH_SIZE = 21;
	const char P_REQUEST_SLOT_FILE = (char)2;
	const unsigned P_REQUEST_SLOT_FILE_SIZE = 21;
	const char P_CLOSE_SLOT = (char)4;
	const unsigned P_CLOSE_SLOT_SIZE = 2;

	//protocol commands server -> client
	const char P_SLOT_ID = (char)3;
	const unsigned P_SLOT_ID_SIZE = 2;
	const char P_WAIT = (char)6;
	const unsigned P_WAIT_SIZE = 1;

	/*
	Special command. This is meant to be sent by a web browser. The response is
	an HTML page with debug info. This is restricted to localhost.

	Note: The full command is variable size and terminated by \n\r but it will
	      always start with 'G' (ascii dec 71) because that's how a GET request
	      starts.
	*/
	const char P_INFO = 'G';

	//largest possible packet (used to determine buffer sizes)
	const unsigned S_MAX_SIZE = P_REQUEST_SLOT_FILE_SIZE;
	const unsigned C_MAX_SIZE = P_BLOCK_TO_CLIENT_SIZE;
}
#endif
