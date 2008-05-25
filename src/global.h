#ifndef H_GLOBALS
#define H_GLOBALS

//uncomment to disable all asserts
//#define NDEBUG

//std
#include <cassert>
#include <iostream>
#include <string>

//custom
#include "logger.h"
#include "sha.h"

namespace global
{
	const std::string NAME = "p2p";
	const std::string VERSION = "0.00 pre-alpha";

	const int MAX_CONNECTIONS = 50;       //maximum number of connections the server will accept
	const int NEW_CONN_THREADS = 2;       //how many threads are in the thread pool for making new client connections
	const int PIPELINE_SIZE = 32;         //how many pre-requests can be done
	const int P2P_PORT = 6969;            //port client connects to and server receives on
	const int GUI_TICK = 100;             //time(in milliseconds) between gui updates
	const int SPEED_AVERAGE = 4;          //how many seconds to average upload/download speed over
	const int COMPLETE_REMOVE = 4;        //how many seconds to wait before removing a complete upload
	const int TIMEOUT = 8;                //how long before an unresponsive server times out
	const int UNRESPONSIVE_TIMEOUT = 60;  //if cannot connect to server, minimum time before retry 
	const int SHARE_REFRESH = 1;          //number of seconds between updating share information
	const int SPINLOCK_TIME = 1000;       //microsecond timeout on all spinlocks (accuracy limited by scheduler tick rate)
	const int UP_SPEED = 9999*1024+512;   //upload speed limit (B/s)
	const int DOWN_SPEED = 9999*1024+512; //download speed limit (B/s)

	//protocol commands client -> server
	const char P_SEND_BLOCK = (char)1;
	const int P_SEND_BLOCK_SIZE = 9;

	//protocol commands for download_file, server -> client
	const char P_FILE_DOES_NOT_EXIST = (char)2;
	const int P_FILE_DOES_NOT_EXIST_SIZE = 1;
	const char P_FILE_NOT_FOUND = (char)3;
	const int P_FILE_NOT_FOUND_SIZE = 1;
	const char P_BLOCK = (char)4;
	const int P_BLOCK_SIZE = 8192;

	//size of the largest possibe packet the server and client can receive
	const int S_MAX_SIZE = P_SEND_BLOCK_SIZE;
	const int C_MAX_SIZE = P_BLOCK_SIZE;

	//default locations
	const std::string CLIENT_DOWNLOAD_DIRECTORY = "download/";
	const std::string SERVER_SHARE_DIRECTORY = "share/";
	const std::string HASH_DIRECTORY = "hash/";
	const std::string DATABASE_PATH = "database";
}
#endif
