#ifndef H_PROTOCOL
#define H_PROTOCOL

//include
#include <SHA1.hpp>

namespace protocol
{
//hard coded protocol preferences
const unsigned TIMEOUT = 16;          //time after which a block will be rerequested
const unsigned DH_KEY_SIZE = 16;      //size exchanged key in Diffie-Hellman-Merkle
const unsigned HASH_BLOCK_SIZE = 512; //number of hashes in hash block
const unsigned FILE_BLOCK_SIZE = HASH_BLOCK_SIZE * SHA1::bin_size;

//commands and messages sizes
const unsigned char ERROR = static_cast<unsigned char>(0);
const unsigned ERROR_SIZE = 1;
const unsigned char REQUEST_SLOT = static_cast<unsigned char>(1);
const unsigned REQUEST_SLOT_SIZE = 21;
const unsigned char SLOT = static_cast<unsigned char>(2);
static const unsigned SLOT_SIZE(const unsigned bit_field_1_size = 0,
	const unsigned bit_field_2_size = 0)
{
	return 31 + bit_field_1_size + bit_field_2_size;
}
const unsigned char REQUEST_HASH_TREE_BLOCK = static_cast<unsigned char>(3);
static unsigned REQUEST_HASH_TREE_BLOCK_SIZE(const unsigned VLI_size)
{
	return 2 + VLI_size;
}
const unsigned char REQUEST_FILE_BLOCK = static_cast<unsigned char>(4);
static unsigned REQUEST_FILE_BLOCK_SIZE(const unsigned VLI_size)
{
	return 2 + VLI_size;
}
const unsigned char BLOCK = static_cast<unsigned char>(5);
static unsigned BLOCK_SIZE(const unsigned block_size)
{
	return 1 + block_size;
}
const unsigned char HAVE_HASH_TREE_BLOCK = static_cast<unsigned char>(6);
static unsigned HAVE_HASH_TREE_BLOCK_SIZE(const unsigned VLI_size)
{
	return 2 + VLI_size;
}
const unsigned char HAVE_FILE_BLOCK = static_cast<unsigned char>(7);
static unsigned HAVE_FILE_BLOCK_SIZE(const unsigned VLI_size)
{
	return 2 + VLI_size;
}
const unsigned char CLOSE_SLOT = static_cast<unsigned char>(8);
const unsigned CLOSE_SLOT_SIZE = 2;
}//end of protocol namespace
#endif
