#include "exchange_tcp.hpp"

//BEGIN send_speed_element
exchange_tcp::send_speed_element::send_speed_element(
	const unsigned remaining_bytes_in,
	const speed_composite & Speed_Calc_in,
	const boost::function<void ()> & call_back_in
):
	remaining_bytes(remaining_bytes_in),
	Speed_Calc(Speed_Calc_in),
	call_back(call_back_in)
{

}

exchange_tcp::send_speed_element::send_speed_element(const send_speed_element & SSE):
	remaining_bytes(SSE.remaining_bytes),
	Speed_Calc(SSE.Speed_Calc),
	call_back(SSE.call_back)
{

}

bool exchange_tcp::send_speed_element::consume(unsigned & n_bytes)
{
	if(n_bytes == 0){
		return false;
	}
	if(n_bytes <= remaining_bytes){
		Speed_Calc.add(n_bytes);
		remaining_bytes -= n_bytes;
		n_bytes = 0;
	}else{
		Speed_Calc.add(remaining_bytes);
		n_bytes -= remaining_bytes;
		remaining_bytes = 0;
	}
	if(remaining_bytes == 0 && call_back){
		call_back();
	}
	return remaining_bytes == 0;
}
//END send_speed_element

exchange_tcp::exchange_tcp(
	net::proactor & Proactor_in,
	net::proactor::connection_info & CI
):
	connection_ID(CI.connection_ID),
	Proactor(Proactor_in),
	blacklist_state(0)
{
	//start key exchange
	if(CI.direction == net::outgoing){
		send(boost::shared_ptr<message_tcp::send::base>(new message_tcp::send::key_exchange_p_rA(
			Encryption.send_p_rA())));
		expect_response(boost::shared_ptr<message_tcp::recv::base>(new message_tcp::recv::key_exchange_rB(
			boost::bind(&exchange_tcp::recv_rB, this, _1, boost::ref(CI)))));
	}else{
		expect_response(boost::shared_ptr<message_tcp::recv::base>(new message_tcp::recv::key_exchange_p_rA(
			boost::bind(&exchange_tcp::recv_p_rA, this, _1, boost::ref(CI)))));
	}
}

void exchange_tcp::expect_response(boost::shared_ptr<message_tcp::recv::base> M)
{
	assert(M);
	Expect_Response.push_back(M);
}

void exchange_tcp::expect_anytime(boost::shared_ptr<message_tcp::recv::base> M)
{
	assert(M);
	Expect_Anytime.push_back(M);
}

void exchange_tcp::expect_anytime_erase(boost::shared_ptr<message_tcp::send::base> M)
{
	assert(M);
	assert(!M->buf.empty());
	for(std::list<boost::shared_ptr<message_tcp::recv::base> >::iterator
		it_cur = Expect_Anytime.begin(), it_end = Expect_Anytime.end();
		it_cur != it_end; ++it_cur)
	{
		if(!*it_cur){
			//shared_ptr already empty from previous expect_anytime_erase() call
			continue;
		}
		if((*it_cur)->expect(M->buf)){
			/*
			Schedule message to be erased. We don't erase element here because it
			might invalidate iterator in recv loop in exchange_tcp::recv_call_back.
			The control flow is complicated here. We may be calling this function
			from a call back done in exchange_tcp::recv_call_back.
			*/
			it_cur->reset();
		}
	}
}

void exchange_tcp::recv_call_back(net::proactor::connection_info & CI)
{
	if(Encryption.ready()){
		Encryption.crypt_recv(CI.recv_buf, CI.recv_buf.size() - CI.latest_recv);
	}

	//process incoming messages
	begin:
	while(!CI.recv_buf.empty()){
		//check if message is expected response
		if(!Expect_Response.empty()){
			if(!Expect_Response.front()){
				Expect_Response.pop_front();
				goto begin;
			}
			message_tcp::recv::status Status = Expect_Response.front()->recv(CI.recv_buf);
			if(Status == message_tcp::recv::blacklist){
				db::table::blacklist::add(CI.ep.IP());
				goto end;
			}else if(Status == message_tcp::recv::complete){
				Expect_Response.pop_front();
				goto begin;
			}else if(Status == message_tcp::recv::incomplete){
				goto end;
			}
		}
		//check if message is expected anytime
		for(std::list<boost::shared_ptr<message_tcp::recv::base> >::iterator
			it_cur = Expect_Anytime.begin(), it_end = Expect_Anytime.end();
			it_cur != it_end;)
		{
			if(*it_cur){
				message_tcp::recv::status Status = (*it_cur)->recv(CI.recv_buf);
				if(Status == message_tcp::recv::blacklist){
					db::table::blacklist::add(CI.ep.IP());
					goto end;
				}else if(Status == message_tcp::recv::complete){
					goto begin;
				}else if(Status == message_tcp::recv::incomplete){
					goto end;
				}else{
					++it_cur;
				}
			}else{
				//message sceduled to be erased
				it_cur = Expect_Anytime.erase(it_cur);
			}
		}
		//a message was not processed, message on front of recv_buf is not expected
		db::table::blacklist::add(CI.ep.IP());
		goto end;
	}
	end:

	//receive may have caused host to be blacklisted
	if(db::table::blacklist::modified(blacklist_state)
		&& db::table::blacklist::is_blacklisted(CI.ep.IP()))
	{
		LOG << "blacklist " << CI.ep.IP();
		Proactor.disconnect(connection_ID);
		CI.recv_call_back.clear();
		return;
	}
}

bool exchange_tcp::recv_p_rA(const net::buffer & buf,
	net::proactor::connection_info & CI)
{
	if(!Encryption.recv_p_rA(buf)){
		return false;
	}
	send(boost::shared_ptr<message_tcp::send::base>(new message_tcp::send::key_exchange_rB(
		Encryption.send_rB())));
	//unencrypt any remaining buffer
	Encryption.crypt_recv(CI.recv_buf);
	send_buffered();
	return true;
}

bool exchange_tcp::recv_rB(const net::buffer & buf, net::proactor::connection_info & CI)
{
	Encryption.recv_rB(buf);
	//unencrypt any remaining buffer
	Encryption.crypt_recv(CI.recv_buf);
	send_buffered();
	return true;
}

void exchange_tcp::send(boost::shared_ptr<message_tcp::send::base> M,
	boost::function<void ()> func)
{
	assert(M);
	assert(!M->buf.empty());
	if(!Encryption.ready() && M->encrypt()){
		//buffer messages sent before key exchange complete
		Encrypt_Buf.push_back(M);
	}else if(Encryption.ready() && M->encrypt()){
		//send encrypted message
		Encryption.crypt_send(M->buf);
		Send_Speed.push_back(send_speed_element(M->buf.size(), M->Upload_Speed, func));
		Proactor.send(connection_ID, M->buf);
	}else if(!M->encrypt()){
		//sent unencrypted message
		Send_Speed.push_back(send_speed_element(M->buf.size(), M->Upload_Speed, func));
		Proactor.send(connection_ID, M->buf);
	}
}

void exchange_tcp::send_buffered()
{
	for(std::list<boost::shared_ptr<message_tcp::send::base> >::iterator
		it_cur = Encrypt_Buf.begin(), it_end = Encrypt_Buf.end();
		it_cur != it_end; ++it_cur)
	{
		send(*it_cur);
	}
	Encrypt_Buf.clear();
}

void exchange_tcp::send_call_back(net::proactor::connection_info & CI)
{
	while(!Send_Speed.empty()){
		if(Send_Speed.front().consume(CI.latest_send)){
			Send_Speed.pop_front();
		}else{
			break;
		}
	}
	assert(CI.latest_send == 0);
}
