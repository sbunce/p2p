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

/* Make header diagram like this
    0                   1                   2                   3   
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Source Port          |       Destination Port        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Acknowledgment Number                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

	/* Headers
	Unreliable UDP
		command (1 byte) (int value = 0)
		size (2 bytes) (size of header + data)

	Unreliable UDP Ack

	*/

	/* IDEAS
	-Do not have max_seg_size. Force accepting of anything up to 65535.
	-Consider adding the ability to have messages composed of multiple packets.
	*/

};
}//end namespace net
#endif
