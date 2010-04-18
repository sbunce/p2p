#ifndef H_K_BUCKET
#define H_K_BUCKET

//custom
#include "protocol_udp.hpp"

//include
#include <network/network.hpp>

//standard
#include <ctime>
#include <list>
#include <string>

class k_bucket
{
public:
	k_bucket();

	/*
	erase:
		Remove element with corresponding endpoint.
	exists:
		Returns true if contact with specified endpoint exists in k_bucket.
	ping:
		Returns a set of endpoints which need to be pinged.
	find_node:
		
	size:
		Returns number of contacts in k_bucket.
	update:
		Update last seen time of the contact. If contact not in k_bucket attempt
		to add it. Return true if contact was updated or added, false if k_bucket
		was full.
		Note: This function is allowed to change the remote_ID.
	*/
	void erase(const network::endpoint & endpoint);
	bool exists(const network::endpoint & endpoint);
	std::vector<network::endpoint> ping();

//IDEA, make set containing results stay bucket_size big. Since a set is sorted low to
//high we can always pop the back when we exceed bucket_size elements.
//calculate remote bucket number after getting top 26
	//void find_node(const std::string & remote_ID, const std::string & ID_to_find,
		//std::map<mpint, std::pair<std::string, network::endpoint> & hosts);

	unsigned size();
	bool update(const std::string remote_ID, const network::endpoint & endpoint);

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

		/*
		need_ping:
			Returns true if ping needs to be sent.
		timed_out:
			Returns true if contact has timed out and needs to be removed.
		*/
		bool need_ping();
		bool timed_out();

	private:
		std::time_t last_seen;
		bool ping_sent;
	};

	std::list<contact> Bucket;
};
#endif
