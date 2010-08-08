#include "exchange_udp.hpp"

//BEGIN expect_response_element
exchange_udp::expect_response_element::expect_response_element(
	boost::shared_ptr<message_udp::recv::base> message_in,
	boost::function<void()> timeout_call_back_in
):
	message(message_in),
	timeout_call_back(timeout_call_back_in),
	time_first_expected(std::time(NULL))
{

}

bool exchange_udp::expect_response_element::timed_out()
{
	return std::time(NULL) - time_first_expected > protocol_udp::response_timeout;
}
//END expect_response_element

exchange_udp::exchange_udp()
{
	//setup UDP listener
	std::set<net::endpoint> E = net::get_endpoint(
		"",
		db::table::prefs::get_port()
	);
	assert(!E.empty());
	ndgram.open(*E.begin());
	assert(ndgram.is_open());
}

void exchange_udp::check_timeouts()
{
	for(std::multimap<net::endpoint, expect_response_element>::iterator
		it_cur = Expect_Response.begin(), it_end = Expect_Response.end();
		it_cur != it_end;)
	{
		if(it_cur->second.timed_out()){
			if(it_cur->second.timeout_call_back){
				it_cur->second.timeout_call_back();
			}
			Expect_Response.erase(it_cur++);
		}else{
			++it_cur;
		}
	}
}

unsigned exchange_udp::download_rate()
{
	return Download.speed();
}

void exchange_udp::expect_anytime(boost::shared_ptr<message_udp::recv::base> M)
{
	Expect_Anytime.push_back(M);
}

void exchange_udp::expect_anytime_remove(boost::shared_ptr<message_udp::send::base> M)
{
	for(std::list<boost::shared_ptr<message_udp::recv::base> >::iterator
		it_cur = Expect_Anytime.begin(), it_end = Expect_Anytime.end();
		it_cur != it_end;)
	{
		if((*it_cur)->expect(M->buf)){
			it_cur = Expect_Anytime.erase(it_cur);
		}else{
			++it_cur;
		}
	}
}

void exchange_udp::expect_response(boost::shared_ptr<message_udp::recv::base> M,
	const net::endpoint & endpoint,
	boost::function<void()> timeout_call_back)
{
	Expect_Response.insert(std::make_pair(endpoint,
		expect_response_element(M, timeout_call_back)));
}

void exchange_udp::tick()
{
	//wait for message to arrive
	std::set<int> read, write;
	read.insert(ndgram.socket());
	select(read, write, 1000);

	if(read.empty()){
		//nothing received
		check_timeouts();
		return;
	}

	//recv message
	boost::optional<net::endpoint> from;
	net::buffer recv_buf;
	int n_bytes = ndgram.recv(recv_buf, from);
	if(n_bytes > 0){
		Download.add(n_bytes);
	}else{
		LOG << "recv error";
		exit(1);
	}
	assert(from);

	//check if expected response
	std::pair<std::multimap<net::endpoint, expect_response_element>::iterator,
		std::multimap<net::endpoint, expect_response_element>::iterator >
		range = Expect_Response.equal_range(*from);
	for(; range.first != range.second; ++range.first){
		if(range.first->second.message->recv(recv_buf, *from)){
			Expect_Response.erase(range.first);
			return;
		}

	}

	//check if expected anytime
	for(std::list<boost::shared_ptr<message_udp::recv::base> >::iterator
		it_cur = Expect_Anytime.begin(), it_end = Expect_Anytime.end();
		it_cur != it_end; ++it_cur)
	{
		if((*it_cur)->recv(recv_buf, *from)){
			return;
		}
	}

	check_timeouts();

	//try to send any unsent messages
	while(!Send_Queue.empty()){
		int n_bytes = ndgram.send(Send_Queue.front().first->buf, Send_Queue.front().second);
		if(n_bytes > 0){
			Upload.add(n_bytes);
			Send_Queue.pop_front();
		}else{
			if(ndgram.is_open()){
				LOG << "OS buffers full";
				break;
			}else{
				LOG << "UDP send error";
				exit(1);
			}
		}

		if(n_bytes <= 0){

		}else{

		}
	}
}

void exchange_udp::send(boost::shared_ptr<message_udp::send::base> M,
	const net::endpoint & ep)
{
	assert(!M->buf.empty());
	int n_bytes = ndgram.send(M->buf, ep);
	if(n_bytes > 0){
		Upload.add(n_bytes);
	}else{
		if(ndgram.is_open()){
			LOG << "OS buffers full";
			Send_Queue.push_back(std::make_pair(M, ep));
		}else{
			LOG << "UDP send error";
			exit(1);
		}
	}
}

unsigned exchange_udp::upload_rate()
{
	return Upload.speed();
}
