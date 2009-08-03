#ifndef H_PROTOCOL
#define H_PROTOCOL

namespace protocol
{
//protocol dependent settings
const unsigned DH_KEY_SIZE = 16;   //size of key exchanged with diffie-hellman (bytes)
const unsigned PIPELINE_SIZE = 16; //max pre-requests that can be done (must be >= 2)

/*
HASH_BLOCK_SIZE - number of hashes in a child node in the hash tree.
	Note: HASH_BLOCK_SIZE % 2 must = 0 and should be >= 2.
	Note: A hash block can be smaller than HASH_BLOCK_SIZE.
FILE_BLOCK_SIZE - number of bytes in a file block.
*/
const unsigned HASH_SIZE = 20;        //size of one hash (binary, bytes)
const unsigned HEX_HASH_SIZE = 40;    //size of one hash (hex, bytes)
const unsigned HASH_BLOCK_SIZE = 256; //number of hashes in hash block
const unsigned FILE_BLOCK_SIZE = HASH_BLOCK_SIZE * HASH_SIZE;

//request commands
const unsigned char P_REQUEST_SLOT_HASH_TREE = static_cast<unsigned char>(0);
const unsigned P_REQUEST_SLOT_HASH_TREE_SIZE = 21;
const unsigned char P_REQUEST_SLOT_FILE = static_cast<unsigned char>(1);
const unsigned P_REQUEST_SLOT_FILE_SIZE = 21;
const unsigned char P_REQUEST_BLOCK = static_cast<unsigned char>(2);
const unsigned P_REQUEST_BLOCK_SIZE = 10;
const unsigned char P_CLOSE_SLOT = static_cast<unsigned char>(3);
const unsigned P_CLOSE_SLOT_SIZE = 2;

//response commands
const unsigned char P_SLOT = static_cast<unsigned char>(4);
const unsigned P_SLOT_SIZE = 2;
const unsigned char P_BLOCK = static_cast<unsigned char>(5);
const unsigned P_BLOCK_SIZE = FILE_BLOCK_SIZE + 1;
const unsigned char P_WAIT = static_cast<unsigned char>(6);
const unsigned P_WAIT_SIZE = 1;
const unsigned char P_ERROR = static_cast<unsigned char>(7);
const unsigned P_ERROR_SIZE = 1;

//largest possible message
const unsigned MAX_MESSAGE_SIZE = P_BLOCK_SIZE;
}//end of protocol namespace
#endif
