#ifndef H_GLOBALS
#define H_GLOBALS

//uncomment to disable all asserts
//#define NDEBUG

//uncomment to enable file block corruption test
//#define CORRUPT_BLOCKS

//uncomment to enable resolution of host names
#define RESOLVE_HOST_NAMES

//std
#include <cassert>
#include <string>

//custom
#include "logger.h"
#include "sha.h"

namespace global
{
	const std::string NAME = "p2p";
	const std::string VERSION = "0.00 pre-alpha";

	//default settings
	const int MAX_CONNECTIONS = 500;      //maximum number of connections the server will accept
	const int UP_SPEED = 9999*1024+512;   //upload speed limit (B/s)
	const int DOWN_SPEED = 9999*1024+512; //download speed limit (B/s)

	//hard settings
	const int PIPELINE_SIZE = 16;         //how many pre-requests can be done
	const int RE_REQUEST_TIMEOUT = 8;     //seconds before a file block is re-requested
	const int P2P_PORT = 6969;            //port client connects to and server receives on
	const int GUI_TICK = 100;             //time(in milliseconds) between gui updates
	const int SPEED_AVERAGE = 4;          //how many seconds to average upload/download speed over
	const int TIMEOUT = 16;               //how long before an unresponsive socket times out
	const int UNRESPONSIVE_TIMEOUT = 60;  //if connection to server fails, new connection attempts to it are stopped for this time

	//default locations
	const std::string DOWNLOAD_DIRECTORY = "download/";
	const std::string SHARE_DIRECTORY = "share/";
	const std::string HASH_DIRECTORY = "hash/";
	const std::string DATABASE_PATH = "database";

	//protocol commands client <-> server
	const char P_ERROR = (char)0;

	//protocol commands client -> server
	const char P_REQUEST_SLOT_FILE = (char)1;
	const int P_REQUEST_SLOT_FILE_SIZE = 21;
	const char P_REQUEST_SLOT_HASH = (char)2;
	const int P_REQUEST_SLOT_HASH_SIZE = 21;
	const char P_CLOSE_SLOT = (char)3;
	const int P_CLOSE_SLOT_SIZE = 2;
	const char P_SEND_BLOCK = (char)4;
	const int P_SEND_BLOCK_SIZE = 10;

	//protocol commands server -> client
	const char P_SLOT_ID = (char)6;
	const int P_SLOT_ID_SIZE = 2;
	const char P_BLOCK = (char)7;
	const int P_BLOCK_SIZE = 10241;

	//largest possible packet (used to determine buffer sizes)
	const int S_MAX_SIZE = P_REQUEST_SLOT_FILE_SIZE;
	const int C_MAX_SIZE = P_BLOCK_SIZE;

	/*
	This is the size of a file block (hash block or file block) sent in a P_BLOCK
	response. It is also the size that files are hashed in.

	WARNING: FILE_BLOCK_SIZE % sha::HASH_LENGTH must equal 0
	         FILE_BLOCK_SIZE % 2 must equal 0
	*/
	const int FILE_BLOCK_SIZE = P_BLOCK_SIZE - 1;
}
#endif
