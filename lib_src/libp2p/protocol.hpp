#ifndef H_PROTOCOL
#define H_PROTOCOL

namespace protocol
{
const unsigned TIMEOUT = 16;          //time after which a block will be rerequested
const unsigned DH_KEY_SIZE = 16;      //size exchanged key in Diffie-Hellman-Merkle
const unsigned HASH_SIZE = 20;        //size of one hash (binary, bytes)
const unsigned HEX_HASH_SIZE = 40;    //size of one hash (hex, bytes)
const unsigned HASH_BLOCK_SIZE = 512; //number of hashes in hash block
const unsigned FILE_BLOCK_SIZE = HASH_BLOCK_SIZE * HASH_SIZE;

//commands
const unsigned char REQUEST_SLOT = static_cast<unsigned char>(6);
const unsigned REQUEST_SLOT_SIZE = 21;
const unsigned char CLOSE_SLOT = static_cast<unsigned char>(5);
const unsigned CLOSE_SLOT_SIZE = 2;
const unsigned char SLOT_ID = static_cast<unsigned char>(4);
const unsigned SLOT_ID_SIZE = 2;
const unsigned char REQUEST_SLOT_FAILED = static_cast<unsigned char>(3);
const unsigned REQUEST_SLOT_FAILED_SIZE = 21;
const unsigned char REQUEST_BLOCK_HASH_TREE = static_cast<unsigned char>(2);
const unsigned REQUEST_BLOCK_HASH_TREE_SIZE = 6;
const unsigned char REQUEST_BLOCK_FILE = static_cast<unsigned char>(1);
const unsigned REQUEST_BLOCK_FILE_SIZE = 6;
const unsigned char BLOCK = static_cast<unsigned char>(0);
const unsigned BLOCK_SIZE = FILE_BLOCK_SIZE + 1;
}//end of protocol namespace
#endif
