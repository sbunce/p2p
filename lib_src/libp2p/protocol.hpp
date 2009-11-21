#ifndef H_PROTOCOL
#define H_PROTOCOL

namespace protocol
{
const unsigned TIMEOUT = 16;          //time after which a block will be rerequested
const unsigned DH_KEY_SIZE = 16;      //size exchanged key in Diffie-Hellman-Merkle
const unsigned BIN_HASH_SIZE = 20;    //size of one hash (binary, bytes)
const unsigned HEX_HASH_SIZE = 40;    //size of one hash (hex, bytes)
const unsigned HASH_BLOCK_SIZE = 512; //number of hashes in hash block
const unsigned FILE_BLOCK_SIZE = HASH_BLOCK_SIZE * BIN_HASH_SIZE;

//commands
const unsigned char REQUEST_SLOT = static_cast<unsigned char>(0);
const unsigned REQUEST_SLOT_SIZE = 21;
const unsigned char REQUEST_SLOT_FAILED = static_cast<unsigned char>(1);
const unsigned REQUEST_SLOT_FAILED_SIZE = 1;
const unsigned char REQUEST_BLOCK_HASH_TREE = static_cast<unsigned char>(2);
static unsigned REQUEST_BLOCK_HASH_TREE_SIZE(const unsigned VLI_size)
{
	return 2 + VLI_size;
}
const unsigned char REQUEST_BLOCK_FILE = static_cast<unsigned char>(3);
static unsigned REQUEST_BLOCK_FILE_SIZE(const unsigned VLI_size)
{
	return 2 + VLI_size;
}
const unsigned char CLOSE_SLOT = static_cast<unsigned char>(4);
const unsigned CLOSE_SLOT_SIZE = 2;
const unsigned char FILE_REMOVED = static_cast<unsigned char>(5);
const unsigned FILE_REMOVED_SIZE = 1;
const unsigned char SLOT_ID = static_cast<unsigned char>(6);
const unsigned SLOT_ID_SIZE = 31;
const unsigned char SUBSCRIBE_HASH_TREE_CHANGES = static_cast<unsigned char>(7);
const unsigned SUBSCRIBE_HASH_TREE_CHANGES_SIZE = 2;
const unsigned char SUBSCRIBE_FILE_CHANGES = static_cast<unsigned char>(8);
const unsigned SUBSCRIBE_FILE_CHANGES_SIZE = 2;
const unsigned char BIT_FIELD = static_cast<unsigned char>(9);
static unsigned BIT_FIELD_SIZE(const unsigned bit_field_size)
{
	return 1 + bit_field_size;
}
const unsigned char BIT_FIELD_COMPLETE = static_cast<unsigned char>(10);
static unsigned BIT_FIELD_COMPLETE_SIZE = 1;
const unsigned char BLOCK = static_cast<unsigned char>(11);
static unsigned BLOCK_SIZE(const unsigned block_size)
{
	return 1 + block_size;
}
const unsigned char HAVE_HASH_TREE_BLOCK = static_cast<unsigned char>(12);
static unsigned HAVE_HASH_TREE_BLOCK_SIZE(const unsigned VLI_size)
{
	return 2 + VLI_size;
}
const unsigned char HAVE_FILE_BLOCK = static_cast<unsigned char>(13);
static unsigned HAVE_FILE_BLOCK_SIZE(const unsigned VLI_size)
{
	return 2 + VLI_size;
}
}//end of protocol namespace
#endif
