#ifndef H_GLOBALS
#define H_GLOBALS

#include <string>
#include "sha.h"

namespace global //"global" namespace ;-)
{
	/* *********** BEGIN DEBUGGING OPTIONS *********** */
	#define DEBUG
	//#define DEBUG_VERBOSE
	//#define REREQUEST_PERCENTAGE

	//what % of fileBlocks to "drop"
	//#define UNRELIABLE_CLIENT
	const int UNRELIABLE_CLIENT_VALUE = 95;

	/*
	If enabled server sometimes sends 1 byte more than it should. The abusive server
	value is the number of packets to process before simulating abuse.
	*/
	//#define ABUSIVE_SERVER
	const int ABUSIVE_SERVER_VALUE = 20000;
	/* ************ END DEBUGGING OPTIONS ************ */

	const int BUFFER_SIZE = 1024;     //size of one packet
	const int SUPERBLOCK_SIZE = 2048; //how many fileBlocks are kept in a superBlock
	const int MAX_CONNECTIONS = 50;   //maximum number of connections both incoming/outgoing
	const int P2P_PORT = 6969;        //port client connects to and server receives on
	const int GUI_TICK = 100;         //time(in milliseconds) between gui updates
	const int SPEED_AVERAGE = 5;      //how many seconds to average upload/download speed over
	const int COMPLETE_REMOVE = 5;    //how many seconds to wait before removing a complete upload/download
	const int TIMEOUT = 25;           //server timeout will be between 1X and 2X this value
	const int SHARE_REFRESH = 1200;   //number of seconds between updating share information

	/*
	C_CTRL_SIZE includes a command and potentially request data.
	example request: X Y Y Y Y Z Z Z Z are the 9 bits of control data.
	X - command byte
	Y - file ID bytes
	Z - block number bytes
	S_CTRL_SIZE includes only a command.
	*/
	const int C_CTRL_SIZE = 9; //protocol control data size, client -> server
	const int S_CTRL_SIZE = 1; //protocol control data size, server -> client

	//protocol commands, client -> server
	const char P_SBL = (char)1; //"send a file block"

	//protocol commands, server -> client
	const char P_DNE = (char)2; //"I don't have that block yet but I have the file"
	const char P_FNF = (char)3; //"I don't have that file"
	const char P_BLS = (char)4; //"here is the block requested"

	//protocol commands, client and server
	const char P_REJ = (char)5; //"I have too many connections already"

	//if this is changed the database will need to be rebuilt
	const sha::SHA_TYPE HASH_TYPE = sha::enuSHA1;

	//default locations
	const std::string CLIENT_DOWNLOAD_DIRECTORY = "download/";
	const std::string SERVER_SHARE_DIRECTORY = "share/";
	const std::string DATABASE_PATH = "database";
}
#endif

