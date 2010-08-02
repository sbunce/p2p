#ifndef H_NET_NRDGRAM
#define H_NET_NRDGRAM

//custom
#include "ndgram.hpp"

//standard
#include <map>

namespace net{
class nrdgram : public socket_base
{
public:
	nrdgram()
	{

	}

	//open for communication to specific endpoint
	virtual void open(const endpoint & E)
	{

	}

private:
	/* Max Safe UDP/IPv4 MTU
	RFC 791 (IP) specifies that all hosts must be able to accept packets of 576
	bytes. RFC 768 (UDP) specifies the UDP header is 8 bytes. RFC 791 specifies
	the max IP header size is 60 bytes.

	From this we can conclude the maximum amount of data we can send.
	576 - (60 + 8) = 508 bytes
	*/

	/* Max Safe UDP/IPv6 MTU
	RFC 2460 (IPv6) specifies that all hosts must be able to accept packets of
	1500 bytes. RFC 768 (UDP) specifies the UDP header is 8 bytes. RFC 2460
	spepcifies the max IPv6 header size is 80 bytes.

	From this we can conclude the maximum amount of data we can send.
	1500 - (80 + 8) = 1412
	*/

	/*
	Have unreliable datagram and reliable datagram. Support messages that are
	fragmented and reassembled on the ends so it's not necessary for the program
	to know about MTU unless it wants to.
	*/

	/* Headers
	We encapsulate the RUDP header inside the IP/UDP header. So basically this is
	the IP/UDP/RUDP header. This first byte is treated as an unsigned integer.
	This byte specifies what kind of header will follow.

	Unreliable UDP
	+---+---+---+---+
	| 0 |    data   |
	+---+---+---+---+
	  0   1  ...  n

	Reliable UDP
	+---+---+---+---+---+---+
	| 1 |seq_num|    data   |
	+---+---+---+---+---+---+
	  0   1   2   4  ...  n

	*/
};
}//end namespace net
#endif
