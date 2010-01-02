#include "exchange.hpp"

exchange::exchange(
	network::proactor & Proactor_in,
	network::connection_info & CI
):
	connection_ID(CI.connection_ID),
	Proactor(Proactor_in)
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
			might invalidate iterator in recv loop in proactor_recv_call_back.
			*/
			(*iter_cur)->func.clear();
		}
	}
}

void exchange::recv_call_back(network::connection_info & CI)
{
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
	if(CI.recv_buf.empty()){
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
	//check if message is not a response
	for(std::list<boost::shared_ptr<message::base> >::iterator
		iter_cur = Expect_Anytime.begin(), iter_end = Expect_Anytime.end();
		iter_cur != iter_end;)
	{
		if((*iter_cur)->func){
			if((*iter_cur)->expects(CI.recv_buf)){
				if((*iter_cur)->recv(CI)){
					(*iter_cur)->func(*iter_cur);
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
		Proactor.send(connection_ID, M->buf);
	}else if(!M->encrypt()){
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