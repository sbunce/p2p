#ifndef H_GLOBAL
#define H_GLOBAL

//uncomment to disable all asserts, and special debug code
//#define NDEBUG

//testing features, uncomment to enable
//#define DISABLE_ENCRYPTION
//#define CORRUPT_HASH_BLOCK_TEST
//#define CORRUPT_FILE_BLOCK_TEST

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

//std
#include <cassert>
#include <limits>
#include <sstream>
#include <string>

#ifdef WIN32
#include <windows.h>
#undef max //this interferes with std::numbeic_limits<int>::max()
#endif

namespace global
{
	const std::string NAME = "p2p";
	const std::string VERSION = "0.00 pre-alpha";

	//default settings, may be changed at run time
	const unsigned MAX_CONNECTIONS = 1024;
	const unsigned DOWNLOAD_RATE = std::numeric_limits<unsigned>::max(); //B/s
	const unsigned UPLOAD_RATE = std::numeric_limits<unsigned>::max();   //B/s

	//hard settings, not changed at runtime
	const unsigned SHARE_SCAN_RATE = 2;        //share scan rate (files/second)
	const unsigned PRIME_CACHE = 32;           //number of primes to keep in prime cache for diffie-hellman
	const unsigned DH_KEY_SIZE = 64;           //size of key exchanged with diffie-hellman (bytes)
	const unsigned PIPELINE_SIZE = 16;         //max pre-requests that can be done (must be >= 2)
	const unsigned P_WAIT_TIMEOUT = 8;         //when a client download_file/download_hash_tree receives a P_WAIT it will not make any requests for this long
	const unsigned P2P_PORT = 6969;            //port client connects to and server receives on
	const unsigned GUI_TICK = 100;             //time(in milliseconds) between gui updates
	const unsigned SPEED_AVERAGE = 8;          //how many seconds to average speed over
	const unsigned RE_REQUEST = 16;            //seconds before a file block is re-requested
	const unsigned RE_REQUEST_FINISHING = 16;  //seconds before a file block is re-requested when download is in finishing phase (last block requested)
	const unsigned TIMEOUT = 64;               //how long before an unresponsive socket times out
	const unsigned UNRESPONSIVE_TIMEOUT = 480; //if connection to server fails, new connection attempts to it are stopped for this time (seconds)

	//default locations
	const std::string DOWNLOAD_DIRECTORY = "download/";
	const std::string SHARE_DIRECTORY = "share/";

	/*
	The size of the file block is always related to the size of a hash block so
	that the same command and function can be used to serve both.

	HASH_BLOCK_SIZE - number of hashes in a child node in the hash tree.
		Note: HASH_BLOCK_SIZE % 2 must = 0 and should be >= 2;
	FILE_BLOCK_SIZE - number of bytes in a file block.
	*/
	const unsigned HASH_SIZE = 20;        //size of one hash (binary)
	const unsigned HEX_HASH_SIZE = 40;    //size of one hash (hex)
	const unsigned HASH_BLOCK_SIZE = 256; //number of hashes in hash block
	const unsigned FILE_BLOCK_SIZE = HASH_BLOCK_SIZE * HASH_SIZE;

	//request commands
	const char P_REQUEST_SLOT_HASH_TREE = (char)0;
	const unsigned P_REQUEST_SLOT_HASH_TREE_SIZE = 21;
	const char P_REQUEST_SLOT_FILE = (char)1;
	const unsigned P_REQUEST_SLOT_FILE_SIZE = 21;
	const char P_REQUEST_BLOCK = (char)2;
	const unsigned P_REQUEST_BLOCK_SIZE = 10;
	const char P_CLOSE_SLOT = (char)3;
	const unsigned P_CLOSE_SLOT_SIZE = 2;

	//response commands
	const char P_SLOT = (char)4;
	const unsigned P_SLOT_SIZE = 2;
	const char P_BLOCK = (char)5;
	const int P_BLOCK_SIZE = FILE_BLOCK_SIZE + 1;
	const char P_WAIT = (char)6;
	const unsigned P_WAIT_SIZE = 1;
	const char P_ERROR = (char)7;
	const unsigned P_ERROR_SIZE = 1;

	//largest possible message
	const unsigned MAX_MESSAGE_SIZE = P_BLOCK_SIZE;

	//function to determine the type of a command
	enum{
		COMMAND_REQUEST,
		COMMAND_RESPONSE,
		COMMAND_INVALID
	};
	static int command_type(const char & command)
	{
		int c_val = (int)(unsigned char)command;
		if(c_val >= 0 && c_val <= 3){
			return COMMAND_REQUEST;
		}else if(c_val >= 4 && c_val <= 7){
			return COMMAND_RESPONSE;
		}else{
			return COMMAND_INVALID;
		}
	}
}
#endif
