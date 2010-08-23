//include
#include <net/net.hpp>
#include <unit_test.hpp>

//standard
#include <csignal>

int fail(0);
const unsigned echo(32);
unsigned echo_cnt(0);
boost::shared_ptr<net::nstream_proactor> Proactor;
boost::mutex mutex;
boost::condition_variable_any cond;

void connect_call_back(net::nstream_proactor::connect_event CE)
{
	if(CE.info->tran != net::nstream_proactor::nstream_tran){
		return;
	}
	if(CE.info->dir == net::nstream_proactor::outgoing_dir){
		Proactor->send(CE.info->conn_ID, net::buffer("x"));
	}
}

void disconnect_call_back(net::nstream_proactor::disconnect_event DE)
{

}

void recv_call_back(net::nstream_proactor::recv_event RE)
{
	assert(RE.buf.size() == 1);
	assert(RE.buf[0] == 'x');
	if(RE.info->dir == net::nstream_proactor::outgoing_dir){
		//this byte was echo'd back to us
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(mutex);
		++echo_cnt;
		if(echo_cnt == echo){
			cond.notify_one();
		}
		}//END lock scope
		Proactor->disconnect(RE.info->conn_ID);
	}else{
		//byte we need to echo back
		Proactor->send(RE.info->conn_ID, RE.buf, true);
	}
}

void send_call_back(net::nstream_proactor::send_event SE)
{

}

int main()
{
	unit_test::timeout();
	Proactor.reset(new net::nstream_proactor(
		&connect_call_back,
		&disconnect_call_back,
		&recv_call_back,
		&send_call_back
	));
	std::set<net::endpoint> E = net::get_endpoint("127.0.0.1", "0");
	assert(!E.empty());
	std::string port = *Proactor->listen(*E.begin());
	assert(!port.empty());
	E = net::get_endpoint("127.0.0.1", port);
	assert(!E.empty());
	for(int x=0; x<echo; ++x){
		Proactor->connect(*E.begin());
	}
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(mutex);
	while(echo_cnt < echo){
		cond.wait(mutex);
	}
	}//END lock scope
	Proactor.reset();
	return fail;
}
