#ifndef H_PROTOCOL
#define H_PROTOCOL

namespace protocol
{
/*
DH_KEY_SIZE:
	Size of the key to be exchanged with Diffie-Hellman-Merkle key exchange
	(bytes).
PIPELINE_SIZE:
	Maximum possible size of the pipeline.
	Note: must be >= 2.
*/
const unsigned DH_KEY_SIZE = 16;
const unsigned PIPELINE_SIZE = 16;

/*
HASH_BLOCK_SIZE:
	Maximum number of hashes in two child nodes in the hash tree.
	Note: HASH_BLOCK_SIZE % 2 must = 0 and should be >= 2.
FILE_BLOCK_SIZE:
	Maximum number of bytes in a file block. All file blocks will be the same
	size except for possibly the last file block.
*/
const unsigned HASH_SIZE = 20;        //size of one hash (binary, bytes)
const unsigned HEX_HASH_SIZE = 40;    //size of one hash (hex, bytes)
const unsigned HASH_BLOCK_SIZE = 512; //number of hashes in hash block
const unsigned FILE_BLOCK_SIZE = HASH_BLOCK_SIZE * HASH_SIZE;

//request commands
const unsigned char REQUEST_SLOT_HASH_TREE = static_cast<unsigned char>(0);
const unsigned REQUEST_SLOT_HASH_TREE_SIZE = 21;
const unsigned char REQUEST_SLOT_FILE = static_cast<unsigned char>(1);
const unsigned REQUEST_SLOT_FILE_SIZE = 21;
const unsigned char REQUEST_BLOCK = static_cast<unsigned char>(2);
const unsigned REQUEST_BLOCK_SIZE = 6;
const unsigned char CLOSE_SLOT = static_cast<unsigned char>(3);
const unsigned CLOSE_SLOT_SIZE = 2;

//response commands
const unsigned char ERROR = static_cast<unsigned char>(4);
const unsigned ERROR_SIZE = 1;
const unsigned char SLOT_ID = static_cast<unsigned char>(5);
const unsigned SLOT_ID_SIZE = 2;
const unsigned char BLOCK = static_cast<unsigned char>(6);
const unsigned BLOCK_SIZE = FILE_BLOCK_SIZE + 1;

//largest possible message
const unsigned MAX_MESSAGE_SIZE = BLOCK_SIZE;

enum type {
	REQUEST,
	RESPONSE,
	INVALID
};

//returns the type of command
static type command_type(const unsigned char C)
{
	if(C >= static_cast<int>(REQUEST_SLOT_HASH_TREE)
		&& C <= static_cast<int>(CLOSE_SLOT))
	{
		return REQUEST;
	}else if(C >= static_cast<int>(ERROR) && C <= static_cast<int>(BLOCK)){
		return RESPONSE;
	}else{
		return INVALID;
	}
}
}//end of protocol namespace
#endif
