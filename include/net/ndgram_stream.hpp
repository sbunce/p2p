#ifndef H_NET_NRDGRAM
#define H_NET_NRDGRAM

//custom
#include "ndgram.hpp"

namespace net{
/*
ndgram_stream multiplexes connections on the same port. It handles reassembling
streams that arrive from multiple hosts.
*/
class ndgram_stream : public socket_base
{
public:
	ndgram_stream()
	{

	}

private:
	/* Max IPv4/UDP MTU
	RFC 791 (IP) specifies that all hosts must be able to accept packets of 576
	bytes. RFC 768 (UDP) specifies the UDP header is 8 bytes. RFC 791 specifies
	the max IP header size is 60 bytes.

	From this we can conclude the maximum amount of data we can send.
	576 - (60 + 8) = 508 bytes
	*/

	/* Max IPv6/UDP MTU
	RFC 2460 (IPv6) specifies that all hosts must be able to accept packets of
	1500 bytes. RFC 768 (UDP) specifies the UDP header is 8 bytes. RFC 2460
	spepcifies the max IPv6 header size is 80 bytes.

	From this we can conclude the maximum amount of data we can send.
	1500 - (80 + 8) = 1412
	*/

	/* Timeout
	RFC 4787 specifies the current best practice is to timeout NAT mappings after
	2 minutes if there is no activity. A keep-alive should be sent such that it
	arrives before this timeout.
	*/

	/* Headers
	We encapsulate the RUDP header inside the IP/UDP header. So basically this is
	the IP/UDP/RUDP header. This first byte is treated as an unsigned integer.
	This byte specifies what kind of header will follow.

	Unreliable Datagram
	+---+---+---+---+
	| 0 |    data   |
	+---+---+---+---+
	  0   1  ...  n

	Connect (like TCP SYN)
	+---+---+---+---+---+---+---+
	| 1 |  conn_ID  |  seq_num  |
	+---+---+---+---+---+---+---+
	  0   1  ...  4   5  ...  8

	Accept Connect (like TCP SYN+ACK)
	+---+---+---+---+---+---+---+
	| 1 |  conn_ID  |  seq_num  |
	+---+---+---+---+---+---+---+
	  0   1  ...  4   5  ...  8
	note: conn_ID must equal conn_ID in Connect message
	note: seq_num must equal seq_num in Connect message

	Stream
	+---+---+---+---+---+---+
	| 1 |seq_num|    data   |
	+---+---+---+---+---+---+
	  0   1   2   4  ...  n
	*/

	ndgram N;
};
}//end namespace net
#endif
