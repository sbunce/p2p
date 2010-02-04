#ifndef H_KADEMLIA
#define H_KADEMLIA

//custom
#include "database.hpp"

//include
#include <network/network.hpp>

class kademlia
{
public:
	kademlia();
	~kademlia();

private:
	boost::thread network_thread;

	class contact
	{
	public:
		std::string node_ID;
		std::string IP;
		std::string port;
/*
		bool timed_out()
		{
			return std::time(NULL) - last_seen > idle_timeout;
		}

		void touch()
		{
			last_seen = std::time(NULL);
		}
*/
	private:
		std::time_t last_seen;
	};

	class bucket
	{
	public:
		//the most recently updated contact is on the front
		std::list<contact> contact_list;
	};

	bucket Bucket[SHA1::bin_size * 8];

	/*
	network_loop:
		Loop to send/recv data.
	*/
	void network_loop();
};
#endif
