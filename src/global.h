#ifndef H_GLOBALS
#define H_GLOBALS

//uncomment to disable all asserts
//#define NDEBUG

//uncomment to enable file block corruption test
//#define CORRUPT_BLOCKS

//uncomment to enable resolution of host names
#define RESOLVE_HOST_NAMES

//uncomment to enable encryption
#define ENCRYPTION

//boost
#include <boost/cstdint.hpp>

//custom
#include "logger.h"

//std
#include <cassert>
#include <limits>
#include <string>

/*
These functions are used instead of platform specific sleep functions.
*/
namespace portable_sleep
{
#ifdef WIN32
#include <windows.h>
#endif

	//sleep for specified number of milliseconds
	inline void ms(const int & milliseconds)
	{
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

#ifdef max
#undef max
#endif

	//default settings
	const int MAX_CONNECTIONS = 1024; //maximum number of connections the server will accept
	const unsigned int UP_SPEED = std::numeric_limits<unsigned int>::max();   //upload speed limit (B/s)
	const unsigned int DOWN_SPEED = std::numeric_limits<unsigned int>::max(); //download speed limit (B/s)

	//hard settings
	const int PRIME_CACHE = 256;         //number of primes to keep in prime cache for diffie-hellman
	const int DH_KEY_SIZE = 64;          //size of key exchanged with diffie-hellman (bytes)
	const int PIPELINE_SIZE = 8;         //how many pre-requests can be done
	const int RE_REQUEST = 16;           //seconds before a file block is re-requested
	const int P_WAIT_TIMEOUT = 8;        //when a client download_file/download_hash_tree receives a P_WAIT it will not make any requests for this long
	const int RE_REQUEST_FINISHING = 4;  //seconds before a file block is re-requested when download is in finishing phase (last block requested)
	const int P2P_PORT = 6969;           //port client connects to and server receives on
	const int GUI_TICK = 100;            //time(in milliseconds) between gui updates
	const int SPEED_AVERAGE = 4;         //how many seconds to average speed over
	const int TIMEOUT = 16;              //how long before an unresponsive socket times out
	const int UNRESPONSIVE_TIMEOUT = 60; //if connection to server fails, new connection attempts to it are stopped for this time (seconds)

	//default locations
	const std::string DOWNLOAD_DIRECTORY = "download/";
	const std::string SHARE_DIRECTORY = "share/";
	const std::string HASH_DIRECTORY = "hash/";
	const std::string DATABASE_PATH = "database";

	//protocol commands client <-> server
	const char P_ERROR = (char)0;
	const int P_ERROR_SIZE = 1;
	const char P_BLOCK = (char)5;
	const int P_BLOCK_TO_CLIENT_SIZE = 5121;
	const int P_BLOCK_TO_SERVER_SIZE = 10;

	//protocol commands client -> server
	const char P_REQUEST_SLOT_HASH = (char)1;
	const int P_REQUEST_SLOT_HASH_SIZE = 21;
	const char P_REQUEST_SLOT_FILE = (char)2;
	const int P_REQUEST_SLOT_FILE_SIZE = 21;
	const char P_CLOSE_SLOT = (char)4;
	const int P_CLOSE_SLOT_SIZE = 2;

	//protocol commands server -> client
	const char P_SLOT_ID = (char)3;
	const int P_SLOT_ID_SIZE = 2;
	const char P_WAIT = (char)6;
	const int P_WAIT_SIZE = 1;

	//largest possible packet (used to determine buffer sizes)
	const int S_MAX_SIZE = P_REQUEST_SLOT_FILE_SIZE;
	const int C_MAX_SIZE = P_BLOCK_TO_CLIENT_SIZE;

	/*
	This is the size of a file block (hash block or file block) sent in a P_BLOCK
	response. It is also the size that files are hashed in.

	WARNING: FILE_BLOCK_SIZE % sha::HASH_LENGTH must equal 0
	*/
	const int FILE_BLOCK_SIZE = P_BLOCK_TO_CLIENT_SIZE - 1;
}
#endif
