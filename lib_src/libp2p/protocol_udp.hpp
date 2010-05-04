#ifndef H_PROTOCOL_UDP
#define H_PROTOCOL_UDP

//include
#include <SHA1.hpp>

namespace protocol_udp
{
//kademlia
const unsigned bucket_count = SHA1::bin_size * 8;
const unsigned bucket_size = 24;
const unsigned timeout = bucket_count * bucket_size;

//commands and message sizes
const unsigned char ping = 0;
const unsigned ping_size = 25;
const unsigned char pong = 1;
const unsigned pong_size = 25;
const unsigned char find_node = 2;
const unsigned find_node_size = 45;
const unsigned char host_list = 4;
const unsigned host_list_size = 27;     //minimum size
const unsigned host_list_elements = 16; //max elements in host_list
}//end of namespace protocol_udp
#endif
