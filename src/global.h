#ifndef H_GLOBALS
#define H_GLOBALS

//std
#include <iostream>
#include <string>

//custom
#include "sha.h"

namespace global
{
	#define DEBUG

	const std::string NAME = "p2p";
	const std::string VERSION = "0.00 pre-alpha";

	const int MAX_CONNECTIONS = 50;       //maximum number of connections the server will accept
	const int NEW_CONN_THREADS = 10;      //how many threads are in the thread pool for making new client connections
	const int PIPELINE_SIZE = 32;         //how many pre-requests can be done
	const int P2P_PORT = 6969;            //port client connects to and server receives on
	const int GUI_TICK = 100;             //time(in milliseconds) between gui updates
	const int SPEED_AVERAGE = 4;          //how many seconds to average upload/download speed over
	const int COMPLETE_REMOVE = 4;        //how many seconds to wait before removing a complete upload
	const int TIMEOUT = 8;                //how long before an unresponsive server times out
	const int SHARE_REFRESH = 1200;       //number of seconds between updating share information
	const int SPINLOCK_TIME = 1000;       //microsecond timeout on all spinlocks
	const int UP_SPEED = 9999*1024+512;   //upload speed limit (B/s)
	const int DOWN_SPEED = 9999*1024+512; //download speed limit (B/s)

	//protocol commands client -> server
	const char P_SEND_BLOCK = (char)1;
	const int P_SEND_BLOCK_SIZE = 9;

	//protocol commands server -> client
	const char P_FILE_DOES_NOT_EXIST = (char)2;
	const int P_FILE_DOES_NOT_EXIST_SIZE = 1;
	const char P_FILE_NOT_FOUND = (char)3;
	const int P_FILE_NOT_FOUND_SIZE = 1;
	const char P_BLOCK = (char)4;
	const int P_BLOCK_SIZE = 8192;

	//size of the largest possibe packet the server and client can receive
	const int S_MAX_SIZE = P_SEND_BLOCK_SIZE;
	const int C_MAX_SIZE = P_BLOCK_SIZE;

	//hash type to be used for hash tree
	const sha::SHA_TYPE HASH_TYPE = sha::enuSHA1;

	//default locations
	const std::string CLIENT_DOWNLOAD_DIRECTORY = "download/";
	const std::string SERVER_SHARE_DIRECTORY = "share/";
	const std::string HASH_DIRECTORY = "hash/";
	const std::string DATABASE_PATH = "database";

	/*
	All debugging messages come through here.
	INFO  - notable but not bad
	ERROR - bad but not really bad
	FATAL - really bad the program cannot continue

	example usage: global::debug_message(global::INFO,__FILE__,__FUNCTION__,"specifics of message");
	*/
	enum debug_message_type { INFO, ERROR, FATAL };

	inline void print_type(debug_message_type type)
	{
		if(type == INFO){ std::cout <<       "info: "; }
		else if(type == ERROR){ std::cout << "error: "; }
		else if(type == FATAL){ std::cout << "fatal: "; }
	}
	inline void debug_message(debug_message_type type, std::string file,
		std::string function, std::string detail)
	{
		#ifdef DEBUG
		print_type(type);
		std::cout << file << " " << function << " " << detail << std::endl;
		if(type == FATAL){ exit(1); }
		#endif
	}
	template<class opt_param_0> 
	inline void debug_message(debug_message_type type, std::string file,
		std::string function, std::string detail, opt_param_0 Opt_param_0)
	{
		#ifdef DEBUG
		print_type(type);
		std::cout << file << " " << function << " " << detail << Opt_param_0
			<< std::endl;
		if(type == FATAL){ exit(1); }
		#endif
	}
	template<class opt_param_0, class opt_param_1>
	inline void debug_message(debug_message_type type, std::string file,
		std::string function, std::string detail, opt_param_0 Opt_param_0,
		opt_param_1 Opt_param_1)
	{
		#ifdef DEBUG
		print_type(type);
		std::cout << file << " " << function << " " << detail << Opt_param_0
			<< Opt_param_1 << std::endl;
		if(type == FATAL){ exit(1); }
		#endif
	}
	template<class opt_param_0, class opt_param_1, class opt_param_2>
	inline void debug_message(debug_message_type type, std::string file,
		std::string function, std::string detail, opt_param_0 Opt_param_0,
		opt_param_1 Opt_param_1, opt_param_2 Opt_param_2)
	{
		#ifdef DEBUG
		print_type(type);
		std::cout << file << " " << function << " " << detail << Opt_param_0
			<< Opt_param_1 << Opt_param_2 << std::endl;
		if(type == FATAL){ exit(1); }
		#endif
	}
	template<class opt_param_0, class opt_param_1, class opt_param_2, class opt_param_3>
	inline void debug_message(debug_message_type type, std::string file,
		std::string function, std::string detail, opt_param_0 Opt_param_0,
		opt_param_1 Opt_param_1, opt_param_2 Opt_param_2, opt_param_3 Opt_param_3)
	{
		#ifdef DEBUG
		print_type(type);
		std::cout << file << " " << function << " " << detail << Opt_param_0
			<< Opt_param_1 << Opt_param_2 << Opt_param_3 << std::endl;
		if(type == FATAL){ exit(1); }
		#endif
	}
}
#endif
