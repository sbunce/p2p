#include "exchange_udp.hpp"

exchange_udp::exchange_udp()
{
	//setup UDP listener
	std::set<network::endpoint> E = network::get_endpoint(
		"",
		database::table::prefs::get_port(),
		network::udp
	);
	assert(!E.empty());
	ndgram.open(*E.begin());
	assert(ndgram.is_open());
}

void exchange_udp::expect_anytime(boost::shared_ptr<message_udp::recv::base> M)
{
	Expect_Anytime.push_back(M);
}

void exchange_udp::expect_anytime_remove(boost::shared_ptr<message_udp::send::base> M)
{
	for(std::list<boost::shared_ptr<message_udp::recv::base> >::iterator
		iter_cur = Expect_Anytime.begin(), iter_end = Expect_Anytime.end();
		iter_cur != iter_end;)
	{
		if((*iter_cur)->expect(M->buf)){
			iter_cur = Expect_Anytime.erase(iter_cur);
		}else{
			++iter_cur;
		}
	}
}

void exchange_udp::expect_response(boost::shared_ptr<message_udp::recv::base> M)
{
	Expect_Response.push_back(M);
}

void exchange_udp::recv()
{
	//wait for message to arrive
	std::set<int> read, write;
	read.insert(ndgram.socket());
	select(read, write, 100);
	if(read.empty()){
		//nothing received
		return;
	}

	//recv message
	boost::shared_ptr<network::endpoint> from;
	network::buffer recv_buf;
	ndgram.recv(recv_buf, from);
	assert(from);

	//check if expected response
	for(std::list<boost::shared_ptr<message_udp::recv::base> >::iterator
		iter_cur = Expect_Response.begin(), iter_end = Expect_Response.end();
		iter_cur != iter_end; ++iter_cur)
	{
		if((*iter_cur)->recv(recv_buf)){
			Expect_Response.erase(iter_cur);
			return;
		}
	}

	//check if expected anytime
	for(std::list<boost::shared_ptr<message_udp::recv::base> >::iterator
		iter_cur = Expect_Anytime.begin(), iter_end = Expect_Anytime.end();
		iter_cur != iter_end; ++iter_cur)
	{
		if((*iter_cur)->recv(recv_buf)){
			return;
		}
	}
}

void exchange_udp::send(boost::shared_ptr<message_udp::send::base> M,
	const network::endpoint & endpoint)
{
	ndgram.send(M->buf, endpoint);
}
