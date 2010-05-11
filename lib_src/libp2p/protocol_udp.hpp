#ifndef H_PROTOCOL_UDP
#define H_PROTOCOL_UDP

//include
#include <SHA1.hpp>

namespace protocol_udp
{
/* Kademlia
bucket_count:
	Number of buckets.
	Note: There are two buckets for each bit. One IPv4 and one IPv6 bucket.
bucket_size:
	Maximum active contacts in bucket.
contact_timeout:
	Set timeout such that if all buckets full we have enough time to ping every
	contact before timeout.
response_timeout:
	How long to wait for a response to a request.
max_store:
	Maximum number of endpoints to send store command to.
retransmit_limit:
	Maximum limit for retransmission.
*/
const unsigned bucket_count = SHA1::bin_size * 8;
const unsigned bucket_size = 16;
const unsigned contact_timeout = bucket_count * bucket_size * 2;
const unsigned response_timeout = 60;
const unsigned max_store = 16;
const unsigned retransmit_limit = 4;

//commands and message sizes
const unsigned char ping = 0;
const unsigned ping_size = 25;
const unsigned char pong = 1;
const unsigned pong_size = 25;
const unsigned char find_node = 2;
const unsigned find_node_size = 45;
const unsigned char host_list = 5;
const unsigned host_list_size = 27;     //minimum size
const unsigned host_list_elements = 16; //max elements in host_list
}//end of namespace protocol_udp
#endif
