#ifndef H_PROTOCOL_TCP
#define H_PROTOCOL_TCP

//include
#include <SHA1.hpp>

namespace protocol_tcp
{
//hard coded protocol preferences
const unsigned timeout = 16;            //time after which a block will be rerequested
const unsigned max_block_pipeline = 16; //maximum pipeline size for block requests
const unsigned DH_key_size = 16;        //size exchanged key in Diffie-Hellman-Merkle
const unsigned hash_block_size = 512;   //number of hashes in hash block
const unsigned file_block_size = hash_block_size * SHA1::bin_size;

//commands and messages sizes
const unsigned char error = 0;
const unsigned error_size = 1;
const unsigned char request_slot = 1;
const unsigned request_slot_size = 21;
const unsigned char slot = 2;
static const unsigned slot_size(const unsigned bit_field_1_size,
	const unsigned bit_field_2_size)
{
	return 31 + bit_field_1_size + bit_field_2_size;
}
const unsigned char request_hash_tree_block = 3;
static unsigned request_hash_tree_block_size(const unsigned VLI_size)
{
	return 2 + VLI_size;
}
const unsigned char request_file_block = 4;
static unsigned request_file_block_size(const unsigned VLI_size)
{
	return 2 + VLI_size;
}
const unsigned char block = 5;
static unsigned block_size(const unsigned size)
{
	return 1 + size;
}
const unsigned char have_hash_tree_block = 6;
static unsigned have_hash_tree_block_size(const unsigned VLI_size)
{
	return 2 + VLI_size;
}
const unsigned char have_file_block = 7;
static unsigned have_file_block_size(const unsigned VLI_size)
{
	return 2 + VLI_size;
}
const unsigned char close_slot = 8;
const unsigned close_slot_size = 2;
}//end of protocol namespace
#endif
