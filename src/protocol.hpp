#ifndef H_PROTOCOL
#define H_PROTOCOL

#include "buffer.hpp"

namespace protocol
{
	//request commands
	const char REQUEST_SLOT_HASH_TREE = (char)0;
	const unsigned REQUEST_SLOT_HASH_TREE_SIZE = 21;
	const char REQUEST_SLOT_FILE = (char)1;
	const unsigned REQUEST_SLOT_FILE_SIZE = 21;
	const char REQUEST_BLOCK = (char)2;
	const unsigned REQUEST_BLOCK_SIZE = 10;
	const char CLOSE_SLOT = (char)3;
	const unsigned CLOSE_SLOT_SIZE = 2;

	//response commands
	const char SLOT = (char)4;
	const unsigned SLOT_SIZE = 2;
	const char BLOCK = (char)5;
	const int BLOCK_SIZE = FILE_BLOCK_SIZE + 1;
	const char WAIT = (char)6;
	const unsigned WAIT_SIZE = 1;
	const char ERROR = (char)7;
	const unsigned ERROR_SIZE = 1;

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
