#ifndef H_K_BUCKET
#define H_K_BUCKET

//include
#include <network/network.hpp>

//standard
#include <ctime>
#include <map>
#include <string>

class k_bucket
{
public:
	k_bucket();

private:
	class contact
	{
	public:
		contact(
			const std::string & remote_ID_in,
			const network::endpoint & endpoint_in
		);
		contact(const contact & C);

		const std::string remote_ID;
		const network::endpoint endpoint;

		//set to true after ping sent, when waiting for pong
		bool ping_sent;

		/*
		idle:
			Returns seconds since node seen.
		touch:
			Updates the time the node was last seen.
		*/
		std::time_t idle();
		void touch();

	private:
		std::time_t last_seen;
	};

	std::map<std::string, contact> Bucket;
	std::map<std::string, contact> Bucket_Cache;
};
#endif
