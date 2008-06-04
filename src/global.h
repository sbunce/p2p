#ifndef H_GLOBALS
#define H_GLOBALS

//uncomment to disable all asserts
//#define NDEBUG

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
	const int MAX_CONNECTIONS = 50;       //maximum number of connections the server will accept
	const int UP_SPEED = 9999*1024+512;   //upload speed limit (B/s)
	const int DOWN_SPEED = 9999*1024+512; //download speed limit (B/s)
	const int NEW_CONN_THREADS = 8;       //how many threads are in the thread pool for making new client connections
	const int PIPELINE_SIZE = 32;         //how many pre-requests can be done
	const int P2P_PORT = 6969;            //port client connects to and server receives on
	const int GUI_TICK = 100;             //time(in milliseconds) between gui updates
	const int SPEED_AVERAGE = 4;          //how many seconds to average upload/download speed over
	const int COMPLETE_REMOVE = 4;        //how many seconds to wait before removing a complete upload
	const int TIMEOUT = 8;                //how long before an unresponsive server times out
	const int UNRESPONSIVE_TIMEOUT = 60;  //if connection to server fails, new connection attempts to it are stopped for this time
	const int SHARE_REFRESH = 1;          //number of seconds between updating share information

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
	const char P_SEND_BLOCK = (char)4;
	const int P_SEND_BLOCK_SIZE = 10;
	const char P_SEND_HASH = (char)6;
	const int P_SEND_HASH_SIZE = 10;

	//protocol commands server -> client
	const char P_SLOT_ID = (char)3;
	const int P_SLOT_ID_SIZE = 2;
	const char P_BLOCK = (char)5;
	const int P_BLOCK_SIZE = 5121;
	const char P_HASH = (char)7;
	const int P_HASH_SIZE = 21;

	//largest possible packet (used to determine buffer sizes)
	const int S_MAX_SIZE = P_REQUEST_SLOT_FILE_SIZE;
	const int C_MAX_SIZE = P_BLOCK_SIZE;
}
#endif
