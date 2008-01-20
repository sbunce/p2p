#ifndef H_GLOBALS
#define H_GLOBALS

#include <string>
#include "sha.h"

namespace global
{
	/* *********** BEGIN DEBUGGING OPTIONS *********** */
	#define DEBUG
	//#define UNRELIABLE_CLIENT
	/* ************ END DEBUGGING OPTIONS ************ */

	const std::string NAME = "p2p";
	const std::string VERSION = "0.00 pre-alpha";

	const int MAX_CONNECTIONS = 50; //maximum number of connections the server will accept
	const int P2P_PORT = 6969;      //port client connects to and server receives on
	const int GUI_TICK = 100;       //time(in milliseconds) between gui updates
	const int SPEED_AVERAGE = 4;    //how many seconds to average upload/download speed over
	const int COMPLETE_REMOVE = 5;  //how many seconds to wait before removing a complete upload
	const int TIMEOUT = 25;         //server timeout will be between 1X and 2X this value
	const int SHARE_REFRESH = 1200; //number of seconds between updating share information

	//protocol commands, client -> server

	/* P_SBL is the command to request a file block. The request is 9 bytes.
	XYYYYZZZZ
	X - command byte (P_SBL command)
	Y - file ID (4 bytes)
	Z - block number (4 bytes) */
	const char P_SBL = (char)1;
	const int P_SBL_SIZE = 9;

	//protocol commands, server -> client
	const char P_DNE = (char)2;  //file block does not exist (server still downloading file)
	const char P_FNF = (char)3;  //server does not have file with that ID
	const char P_BLS = (char)4;  //block send (remainder of packet is a file block)
	const int P_BLS_SIZE = 8192; //size of a P_BLS packet (last block packet may be smaller)

	//size of the largest possibe packet the server and client can receive (buffer sizes)
	const int S_MAX_SIZE = P_SBL_SIZE;
	const int C_MAX_SIZE = P_BLS_SIZE;

	//hash type to be used for hash tree
	const sha::SHA_TYPE HASH_TYPE = sha::enuSHA1;

	//default locations
	const std::string CLIENT_DOWNLOAD_DIRECTORY = "download/";
	const std::string SERVER_SHARE_DIRECTORY = "share/";
	const std::string DATABASE_PATH = "database";
}
#endif

