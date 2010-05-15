#ifndef H_K_ROUTE_TABLE
#define H_K_ROUTE_TABLE

//custom
#include "k_bucket.hpp"
#include "db_all.hpp"

//include
#include <atomic_int.hpp>

//standard
#include <algorithm>

class k_route_table
{
public:
	/*
	active_cnt:
		Count of how many active contacts exist in k_buckets.
	route_table_call_back:
		Called back whenever a active node added.
	*/
	k_route_table(
		atomic_int<unsigned> & active_cnt,
		const boost::function<void (const net::endpoint &, const std::string &)> & route_table_call_back
	);

	/*
	add_reserve:
		Add endpoint to reserve. Nodes that go in to a k_bucket start in reserve.
	find_node:
		Returns endpoints closest to ID_to_find. The returned list is suitable to
		pass to the message_udp::send::host_list ctor.
	find_node_local:
		Like the above function but used for local find_node search. This function
		returns distance in addition to endpoint.
	ping:
		Returns endpoint to be pinged.
	recv_pong:
		Call back used when pinging endpoint in Unknown. This call back determines
		what bucket the endpoint belongs in.
	*/
	void add_reserve(const net::endpoint & ep, const std::string & remote_ID = "");
	std::list<net::endpoint> find_node(const net::endpoint & from,
		const std::string & ID_to_find);
	std::multimap<mpa::mpint, net::endpoint> find_node_local(const std::string & ID_to_find);
	boost::optional<net::endpoint> ping();
	void recv_pong(const net::endpoint & from, const std::string & remote_ID);

private:
	const std::string local_ID;

	//k_buckets for IPv4 and IPv6
	boost::scoped_ptr<k_bucket> Bucket_4[protocol_udp::bucket_count];
	boost::scoped_ptr<k_bucket> Bucket_6[protocol_udp::bucket_count];

	class contact
	{
	public:
		explicit contact();
		contact(const contact & C);

		/*
		send:
			Returns true if ping should be sent.
		timeout:
			Returns true if contact has timed out.
			Postcondition: timeout() will return false. The contact is reset to
				original state.
		timeout_count:
			Returns number of times the contact has timed out.
		*/
		bool send();
		bool timeout();
		unsigned timeout_count();

	private:
		/*
		time:
			Initially set to the time the contact was instantiated. After send()
			returns true this is used to check for timeout.
		sent:
			Set to true when send() has returned true. Enables timeout of the
			contact.
		timeout_cnt:
			Returns the number of times this contact has timed out. Used for
			retransmission limit.
		*/
		std::time_t time;
		bool sent;
		unsigned timeout_cnt;
	};

	/*
	Unknown_Active:
		When unknown node is pinged it is placed here, along with the time at
		which it was pinged.
	Unknown_Reserve:
		When we don't know the bucket the endpoint belongs in it's placed here.
	*/
	std::map<net::endpoint, contact> Unknown_Active;
	std::set<net::endpoint> Unknown_Reserve;
};
#endif
