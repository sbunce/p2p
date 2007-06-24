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

	/*
	If enabled the client randomly "drops" packets. The value is the number of
	packets to drop out of 10,000. Example: 25 = 0.0025%.
	*/
	//#define UNRELIABLE_CLIENT
	const int UNRELIABLE_CLIENT_VALUE = 500;

	/*
	If enabled server sometimes sends 1 byte more than it should. The value is 
	the number of packets to be abusive with out of 10,000. Example: 15 = 0.0015%.
	This option will result in the client being abused disconnecting the server.
	This option is actually applied on the client but the effect is the same.
	*/
	//#define ABUSIVE_SERVER
	const int ABUSIVE_SERVER_VALUE = 1;

	/* ************ END DEBUGGING OPTIONS ************ */

	const int BUFFER_SIZE = 4096;    //how big packets can be(bytes, maximum 65535)
	const int CONTROL_SIZE = 32;     //how much of the packet is control data(bytes)
	const int SUPERBLOCK_SIZE = 256; //how many fileBlocks are kept in a superBlock

	const int MAX_CONNECTIONS = 50;  //maximum number of connections to server/from client
	const int P2P_PORT = 6969;       //port client connects to and server receives on
	const int GUI_TICK = 100;        //time(in milliseconds) between gui updates
	const int SPEED_AVERAGE = 5;     //how many seconds to average upload/download speed over
	const int COMPLETE_REMOVE = 5;   //how many seconds to wait before removing a complete upload/download

	const std::string P_SBL = "SBL"; //request from client to server for fileBlock
	const std::string P_BLS = "BLS"; //response from server to client with fileBlock attached
	const std::string P_REJ = "REJ"; //rejected connection from the server
	const std::string P_DNE = "DNE"; //server did not find the requested block
	const std::string P_FNF = "FNF"; //server did not find the requested file
	const std::string P_BCM = "BCM"; //server received unrecognized command

	//hash type: enuSHA1, enuSHA224, enuSHA256, enuSHA384, enuSHA512
	//if this is changed all databases will need to be rebuilt
	const sha::SHA_TYPE HASH_TYPE = sha::enuSHA1;

	//default locations of index files and shared/downloaded files
	const std::string CLIENT_DOWNLOAD_INDEX = "download.db";
	const std::string SERVER_SHARE_INDEX = "share.db";
	const std::string EXPLORATION_SEARCH_DATABASE = "search.db";
	const std::string CLIENT_DOWNLOAD_DIRECTORY = "download/";
	const std::string SERVER_SHARE_DIRECTORY = "share/";

	//field delimiter for all text files
	const std::string DELIMITER = "|";
}
#endif

