#ifndef H_GLOBALS
#define H_GLOBALS

#include <string>
#include "sha.h"

namespace global
{
	#define DEBUG

	const std::string NAME = "p2p";
	const std::string VERSION = "0.00 pre-alpha";

	const int MAX_CONNECTIONS = 50;  //maximum number of connections the server will accept
	const int NEW_CONN_THREADS = 10; //how many threads are in the thread pool for making new client connections
	const int PIPELINE_SIZE = 32;    //how many pre-requests can be done(1 disables pipelining)
	const int REREQUEST_FACTOR = 32; //to what factor the server can be "later than normal" in sending a download_file packet
	const int P2P_PORT = 6969;       //port client connects to and server receives on
	const int GUI_TICK = 100;        //time(in milliseconds) between gui updates
	const int SPEED_AVERAGE = 2;     //how many seconds to average upload/download speed over
	const int COMPLETE_REMOVE = 5;   //how many seconds to wait before removing a complete upload
	const int TIMEOUT = 25;          //server timeout will be between 1X and 2X this value
	const int SHARE_REFRESH = 1200;  //number of seconds between updating share information

	//protocol commands client -> server
	const char P_SBL = (char)1;
	const int P_SBL_SIZE = 9;

	//protocol commands for download_file, server -> client
	const char P_DNE = (char)2;
	const int P_DNE_SIZE = 1;
	const char P_FNF = (char)3;
	const int P_FNF_SIZE = 1;
	const char P_BLS = (char)4;
	const int P_BLS_SIZE = 8192;

	//size of the largest possibe packet the server and client can receive
	const int S_MAX_SIZE = P_SBL_SIZE;
	const int C_MAX_SIZE = P_BLS_SIZE;

	//hash type to be used for hash tree
	const sha::SHA_TYPE HASH_TYPE = sha::enuSHA1;

	//default locations
	const std::string CLIENT_DOWNLOAD_DIRECTORY = "download/";
	const std::string SERVER_SHARE_DIRECTORY = "share/";
	const std::string HASH_DIRECTORY = "hash/";
	const std::string DATABASE_PATH = "database";
}
#endif
