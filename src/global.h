#ifndef H_GLOBALS
#define H_GLOBALS

//std
#include <cassert>
#include <iostream>
#include <string>

//custom
#include "logger.h"
#include "sha.h"

namespace global
{
	static const std::string NAME = "p2p";
	static const std::string VERSION = "0.00 pre-alpha";

	static const int MAX_CONNECTIONS = 50;       //maximum number of connections the server will accept
	static const int NEW_CONN_THREADS = 10;      //how many threads are in the thread pool for making new client connections
	static const int PIPELINE_SIZE = 32;         //how many pre-requests can be done
	static const int P2P_PORT = 6969;            //port client connects to and server receives on
	static const int GUI_TICK = 100;             //time(in milliseconds) between gui updates
	static const int SPEED_AVERAGE = 4;          //how many seconds to average upload/download speed over
	static const int COMPLETE_REMOVE = 4;        //how many seconds to wait before removing a complete upload
	static const int TIMEOUT = 8;                //how long before an unresponsive server times out
	static const int SHARE_REFRESH = 1200;       //number of seconds between updating share information
	static const int SPINLOCK_TIME = 1000;       //microsecond timeout on all spinlocks
	static const int UP_SPEED = 9999*1024+512;   //upload speed limit (B/s)
	static const int DOWN_SPEED = 9999*1024+512; //download speed limit (B/s)

	//protocol commands client -> server
	static const char P_SEND_BLOCK = (char)1;
	static const int P_SEND_BLOCK_SIZE = 9;

	//protocol commands for download_file, server -> client
	static const char P_FILE_DOES_NOT_EXIST = (char)2;
	static const int P_FILE_DOES_NOT_EXIST_SIZE = 1;
	static const char P_FILE_NOT_FOUND = (char)3;
	static const int P_FILE_NOT_FOUND_SIZE = 1;
	static const char P_BLOCK = (char)4;
	static const int P_BLOCK_SIZE = 8192;

	//size of the largest possibe packet the server and client can receive
	static const int S_MAX_SIZE = P_SEND_BLOCK_SIZE;
	static const int C_MAX_SIZE = P_BLOCK_SIZE;

	//hash type to be used for hash tree
	static const sha::SHA_TYPE HASH_TYPE = sha::enuSHA1;

	//default locations
	static const std::string CLIENT_DOWNLOAD_DIRECTORY = "download/";
	static const std::string SERVER_SHARE_DIRECTORY = "share/";
	static const std::string HASH_DIRECTORY = "hash/";
	static const std::string DATABASE_PATH = "database";
}
#endif
