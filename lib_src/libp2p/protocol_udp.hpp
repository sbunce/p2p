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
find_timeout:
	Maximum time to spend on a iterative find job.
max_store:
	Maximum number of endpoints to send store command to.
no_delay_count:
	The number of requests to make immediately when starting to find a node.
	Setting this high will make finds work faster but waste more bandwidth and
	contact more hosts.
response_timeout:
	How long to wait for a response to a request.
retransmit_limit:
	Maximum limit for retransmission.
store_token_incoming_timeout:
	Time limit on store tokens a remote host has given us.
	Note: This is kept shorter than outgoing timeout so that the store token
		doesn't time out while it's sent and on the wire.
store_token_outgoing_timeout:
	Time limit on store tokens we send remote hosts.
*/
const unsigned bucket_count = SHA1::bin_size * 8;
const unsigned bucket_size = 16;
const unsigned bucket_timeout = bucket_count * bucket_size * 2;
const unsigned find_timeout = 60;
const unsigned max_store = 16;
const unsigned no_delay_count = 2;
const unsigned response_timeout = 30;
const unsigned retransmit_limit = 2;
const unsigned store_token_outgoing_timeout = 60 * 8;
const unsigned store_token_incoming_timeout = store_token_outgoing_timeout - 60;

//commands and message sizes
const unsigned char ping = 0;
const unsigned ping_size = 25;
const unsigned char pong = 1;
const unsigned pong_size = 25;
const unsigned char find_node = 2;
const unsigned find_node_size = 45;
const unsigned char host_list = 3;
const unsigned host_list_size = 27;     //minimum size
const unsigned host_list_elements = 16; //max elements in host_list
const unsigned char store_node = 4;
const unsigned store_node_size = 25;
const unsigned char store_file = 5;
const unsigned store_file_size = 45;
}//end of namespace protocol_udp
#endif
