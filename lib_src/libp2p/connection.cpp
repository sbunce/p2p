#include "connection.hpp"

connection::connection(
	network::proactor & Proactor_in,
	network::connection_info & CI
):
	connection_ID(CI.connection_ID),
	IP(CI.IP),
	Proactor(Proactor_in),
	blacklist_state(-1),
	Slot_Manager(
		connection_ID,
		boost::bind(&connection::send, this, _1),
		boost::bind(&connection::expect_response, this, _1),
		boost::bind(&connection::expect_anytime, this, _1),
		boost::bind(&connection::expect_anytime_erase, this, _1)
	)
{
	//start key exchange
	if(CI.direction == network::outgoing){
		send(boost::shared_ptr<message::base>(new message::key_exchange_p_rA(Encryption)));
		expect_response(boost::shared_ptr<message::base>(new message::key_exchange_rB(
			boost::bind(&connection::recv_rB, this, _1, boost::ref(CI)))));
	}else{
		expect_response(boost::shared_ptr<message::base>(new message::key_exchange_p_rA(
			boost::bind(&connection::recv_p_rA, this, _1, boost::ref(CI)))));
	}
	CI.recv_call_back = boost::bind(&connection::proactor_recv_call_back, this, _1);
}

void connection::expect_response(boost::shared_ptr<message::base> M)
{
	assert(M);
LOGGER << typeid(*M).name();
	assert(M->func);
	Expect_Response.push_back(M);
}

void connection::expect_anytime(boost::shared_ptr<message::base> M)
{
	assert(M);
	assert(M->func);
	Expect_Anytime.push_back(M);
}

void connection::expect_anytime_erase(network::buffer buf)
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

void connection::proactor_recv_call_back(network::connection_info & CI)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
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
		Proactor.disconnect(CI.connection_ID);
		CI.recv_call_back.clear();
		return;
	}
}

bool connection::recv_p_rA(boost::shared_ptr<message::base> M,
	network::connection_info & CI)
{
	if(!Encryption.recv_p_rA(M->buf)){
		return false;
	}
	send(boost::shared_ptr<message::base>(new message::key_exchange_rB(Encryption)));
	//unencrypt any remaining buffer
	Encryption.crypt_recv(CI.recv_buf);
	send_initial();
	return true;
}

bool connection::recv_rB(boost::shared_ptr<message::base> M,
	network::connection_info & CI)
{
	Encryption.recv_rB(M->buf);
	//unencrypt any remaining buffer
	Encryption.crypt_recv(CI.recv_buf);
	send_initial();
	return true;
}

bool connection::recv_initial(boost::shared_ptr<message::base> M)
{
	std::string peer_ID = convert::bin_to_hex(std::string(
		reinterpret_cast<const char *>(M->buf.data()), SHA1::bin_size));
	LOGGER << peer_ID;
	Slot_Manager.resume(peer_ID);
	return true;
}

void connection::send(boost::shared_ptr<message::base> M)
{
	assert(M && !M->buf.empty());
	if(M->encrypt()){
		//copy buffer to leave original unencrypted
		network::buffer buf = M->buf;
		Encryption.crypt_send(buf);
		Proactor.send(connection_ID, buf);
	}else{
		Proactor.send(connection_ID, M->buf);
	}
}

void connection::send_initial()
{
	std::string peer_ID = database::table::prefs::get_peer_ID();
	send(boost::shared_ptr<message::base>(new message::initial(peer_ID)));
	expect_response(boost::shared_ptr<message::base>(new message::initial(
		boost::bind(&connection::recv_initial, this, _1))));
}
