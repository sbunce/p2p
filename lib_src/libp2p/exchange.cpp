#include "exchange.hpp"

exchange::exchange(
	boost::mutex & Mutex_in,
	network::proactor & Proactor_in,
	network::connection_info & CI,
	boost::function<void()> exchange_call_back_in
):
	connection_ID(CI.connection_ID),
	Mutex(Mutex_in),
	Proactor(Proactor_in),
	exchange_call_back(exchange_call_back_in),
	blacklist_state(0)
{
	//start key exchange
	if(CI.direction == network::outgoing){
		send(boost::shared_ptr<message::base>(new message::key_exchange_p_rA(Encryption)));
		expect_response(boost::shared_ptr<message::base>(new message::key_exchange_rB(
			boost::bind(&exchange::recv_rB, this, _1, boost::ref(CI)))));
	}else{
		expect_response(boost::shared_ptr<message::base>(new message::key_exchange_p_rA(
			boost::bind(&exchange::recv_p_rA, this, _1, boost::ref(CI)))));
	}
	CI.recv_call_back = boost::bind(&exchange::recv_call_back, this, _1);
	CI.send_call_back = boost::bind(&exchange::send_call_back, this, _1);
}

void exchange::expect_response(boost::shared_ptr<message::base> M)
{
	assert(M);
	assert(M->func);
	Expect_Response.push_back(M);
}

void exchange::expect_anytime(boost::shared_ptr<message::base> M)
{
	assert(M);
	assert(M->func);
	Expect_Anytime.push_back(M);
}

void exchange::expect_anytime_erase(network::buffer buf)
{
	for(std::list<boost::shared_ptr<message::base> >::iterator
		iter_cur = Expect_Anytime.begin(), iter_end = Expect_Anytime.end();
		iter_cur != iter_end; ++iter_cur)
	{
		if((*iter_cur)->expects(buf)){
			/*
			Schedule message to be erased. We don't erase element here because it
			might invalidate iterator in recv loop in exchange::recv_call_back.
			*/
			(*iter_cur)->func.clear();
		}
	}
}

void exchange::process_triggers(const std::set<int> & ID_set)
{
	for(std::set<int>::iterator iter_cur = ID_set.begin(), iter_end = ID_set.end();
		iter_cur != iter_end; ++iter_cur)
	{
		Proactor.trigger(*iter_cur);
	}
}

void exchange::recv_call_back(network::connection_info & CI)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(Encryption.ready()){
		Encryption.crypt_recv(CI.recv_buf, CI.recv_buf.size() - CI.latest_recv);
	}

	/*
	This is a loop which processes incoming messages. It ended up being easier to
	understand with goto's. If there are potentially more messages to process we
	goto begin. If the recv_buf is empty, or we know there is only one incomplete
	message in recv_buf we goto end.
	*/
	//BEGIN loop
	begin:
	if(CI.recv_buf.empty() || CI.latest_recv == 0){
		goto end;
	}
	//check if message is an expected response
	if(!Expect_Response.empty() && Expect_Response.front()->expects(CI.recv_buf)){
		if(Expect_Response.front()->recv(CI)){
			if(!Expect_Response.front()->func(Expect_Response.front())){
				database::table::blacklist::add(CI.IP);
				goto end;
			}
			Expect_Response.pop_front();
			goto begin;
		}else{
			goto end;
		}
	}
	//check if message is not expected response
	for(std::list<boost::shared_ptr<message::base> >::iterator
		iter_cur = Expect_Anytime.begin(), iter_end = Expect_Anytime.end();
		iter_cur != iter_end;)
	{
		if((*iter_cur)->func){
			if((*iter_cur)->expects(CI.recv_buf)){
				if((*iter_cur)->recv(CI)){
					if(!(*iter_cur)->func(*iter_cur)){
						database::table::blacklist::add(CI.IP);
						goto end;
					}
					//clear message for reuse
					(*iter_cur)->buf.clear();
					goto begin;
				}
				goto end;
			}
			++iter_cur;
		}else{
			//message sceduled to be erased
			iter_cur = Expect_Anytime.erase(iter_cur);
		}
	}
	LOGGER << "unrecognized message";
	database::table::blacklist::add(CI.IP);
	end:
	//END loop

	//receive may have caused host to be blacklisted
	if(database::table::blacklist::modified(blacklist_state)
		&& database::table::blacklist::is_blacklisted(CI.IP))
	{
		LOGGER << "disconnecting blacklisted " << CI.IP;
		Proactor.disconnect(connection_ID);
		CI.recv_call_back.clear();
		return;
	}
	exchange_call_back();
}

bool exchange::recv_p_rA(boost::shared_ptr<message::base> M,
	network::connection_info & CI)
{
	if(!Encryption.recv_p_rA(M->buf)){
		return false;
	}
	send(boost::shared_ptr<message::base>(new message::key_exchange_rB(Encryption)));
	//unencrypt any remaining buffer
	Encryption.crypt_recv(CI.recv_buf);
	send_buffered();
	return true;
}

bool exchange::recv_rB(boost::shared_ptr<message::base> M,
	network::connection_info & CI)
{
	Encryption.recv_rB(M->buf);
	//unencrypt any remaining buffer
	Encryption.crypt_recv(CI.recv_buf);
	send_buffered();
	return true;
}

void exchange::send(boost::shared_ptr<message::base> M)
{
	assert(M && !M->buf.empty());
	if(!Encryption.ready() && M->encrypt()){
		Encrypt_Buf.push_back(M);
	}else if(Encryption.ready() && M->encrypt()){
		Encryption.crypt_send(M->buf);
		Send_Speed.push_back(std::make_pair(M->buf.size(), M->Speed_Calculator));
		Proactor.send(connection_ID, M->buf);
	}else if(!M->encrypt()){
		Send_Speed.push_back(std::make_pair(M->buf.size(), M->Speed_Calculator));
		Proactor.send(connection_ID, M->buf);
	}
}

void exchange::send_buffered()
{
	for(std::list<boost::shared_ptr<message::base> >::iterator
		iter_cur = Encrypt_Buf.begin(), iter_end = Encrypt_Buf.end();
		iter_cur != iter_end; ++iter_cur)
	{
		send(*iter_cur);
	}
	Encrypt_Buf.clear();
}

void exchange::send_call_back(network::connection_info & CI)
{
	unsigned latest_send = CI.latest_send;
	while(latest_send){
		assert(!Send_Speed.empty());
		if(latest_send >= Send_Speed.front().first){
			if(Send_Speed.front().second){
				Send_Speed.front().second->add(Send_Speed.front().first);
			}
			latest_send -= Send_Speed.front().first;
			Send_Speed.pop_front();
		}else{
			if(Send_Speed.front().second){
				Send_Speed.front().second->add(latest_send);
			}
			Send_Speed.front().first -= latest_send;
			break;
		}
	}
}
