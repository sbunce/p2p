#ifndef H_K_BUCKET
#define H_K_BUCKET

//custom
#include "k_contact.hpp"
#include "k_func.hpp"
#include "protocol_udp.hpp"

//include
#include <atomic_int.hpp>
#include <boost/optional.hpp>
#include <mpa.hpp>
#include <net/net.hpp>

//standard
#include <ctime>
#include <list>
#include <map>
#include <string>

class k_bucket : private boost::noncopyable
{
public:
	k_bucket(
		atomic_int<unsigned> & active_cnt_in,
		const boost::function<void (const net::endpoint &, const std::string &)> & route_table_call_back_in
	);

	/*
	add_reserve:
		Add node to be considered for routing table inclusion. If the node is
		already in the routing table the last_seen time will be updated.
	exists_active:
		Returns true if endpoint exists in bucket (active or reserve).
	find_node (two parameters):
		Calculates distance on all nodes in k_bucket and adds them to the hosts
		container.
	find_node (three parameters):
		Same as find_node (two parameters) but excludes 'from' endpoint from
		results.
	ping:
		Returns a endpoint which needs to be pinged.
	recv_pong:
		Called when pong received.
	*/
	void add_reserve(const net::endpoint & ep, const std::string remote_ID);
	bool exists(const net::endpoint & ep);
	void find_node(const std::string & ID_to_find,
		std::multimap<mpa::mpint, net::endpoint> & hosts);
	void find_node(const net::endpoint & from, const std::string & ID_to_find,
		std::multimap<mpa::mpint, net::endpoint> & hosts);
	boost::optional<net::endpoint> ping();
	void recv_pong(const net::endpoint & from, const std::string & remote_ID);

private:
	atomic_int<unsigned> & active_cnt;
	const boost::function<void (const net::endpoint &, const std::string &)> route_table_call_back;

	class bucket_element
	{
	public:
		bucket_element(
			const net::endpoint & endpoint_in,
			const std::string & remote_ID_in,
			const k_contact & contact_in
		);
		bucket_element(const bucket_element & BE);

		const net::endpoint endpoint;
		const std::string remote_ID;
		k_contact contact;
	};

	/*
	Contacts in Bucket_Active are pinged to make sure they're still up. When a
	active contact times out a replacement will be gotten from reserve.
	*/
	std::list<bucket_element> Bucket_Active;
	std::list<bucket_element> Bucket_Reserve;
};
#endif
