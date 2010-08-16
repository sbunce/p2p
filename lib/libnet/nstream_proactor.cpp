#include <net/nstream_proactor.hpp>

//BEGIN dispatcher
net::nstream_proactor::dispatcher::dispatcher(
	const boost::function<void (connect_event)> & connect_call_back_in,
	const boost::function<void (disconnect_event)> & disconnect_call_back_in,
	const boost::function<void (recv_event)> & recv_call_back_in,
	const boost::function<void (send_event)> & send_call_back_in
):
	connect_call_back(connect_call_back_in),
	disconnect_call_back(disconnect_call_back_in),
	recv_call_back(recv_call_back_in),
	send_call_back(send_call_back_in),
	job_cnt(0)
{
	for(unsigned x=0; x<threads; ++x){
		workers.create_thread(boost::bind(&dispatcher::dispatch, this));
	}
}

net::nstream_proactor::dispatcher::~dispatcher()
{
	join();
	workers.interrupt_all();
	workers.join_all();
}

void net::nstream_proactor::dispatcher::connect(const connect_event & CE)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(mutex);
	while(Job.size() >= max_buf){
		consumer_cond.wait(mutex);
	}
	boost::function<void ()> func = boost::bind(connect_call_back, CE);
	Job.push_back(std::make_pair(CE.conn_ID, func));
	++job_cnt;
	}//END lock scope
	producer_cond.notify_one();
}

void net::nstream_proactor::dispatcher::disconnect(const disconnect_event & DE)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(mutex);
	while(Job.size() >= max_buf){
		consumer_cond.wait(mutex);
	}
	boost::function<void ()> func = boost::bind(disconnect_call_back, DE);
	Job.push_back(std::make_pair(DE.conn_ID, func));
	++job_cnt;
	}//END lock scope
	producer_cond.notify_one();
}

void net::nstream_proactor::dispatcher::join()
{
	boost::mutex::scoped_lock lock(mutex);
	while(job_cnt > 0){
		empty_cond.wait(mutex);
	}
}

void net::nstream_proactor::dispatcher::recv(const recv_event & RE)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(mutex);
	while(Job.size() >= max_buf){
		consumer_cond.wait(mutex);
	}
	boost::function<void ()> func = boost::bind(recv_call_back, RE);
	Job.push_back(std::make_pair(RE.conn_ID, func));
	++job_cnt;
	}//END lock scope
	producer_cond.notify_one();
}

void net::nstream_proactor::dispatcher::send(const send_event & SE)
{
	{//BEGIN lock scope
	boost::mutex::scoped_lock lock(mutex);
	while(Job.size() >= max_buf){
		consumer_cond.wait(mutex);
	}
	boost::function<void ()> func = boost::bind(send_call_back, SE);
	Job.push_back(std::make_pair(SE.conn_ID, func));
	++job_cnt;
	}//END lock scope
	producer_cond.notify_one();
}

void net::nstream_proactor::dispatcher::dispatch()
{
	while(true){
		std::pair<boost::uint64_t, boost::function<void ()> > p;
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(mutex);
		while(Job.empty()){
			producer_cond.wait(mutex);
		}
		for(std::list<std::pair<boost::uint64_t, boost::function<void ()> > >::iterator
			it_cur = Job.begin(); it_cur != Job.end();)
		{
			if(memoize.insert(it_cur->first).second){
				p = *it_cur;
				Job.erase(it_cur);
				break;
			}
		}
		}//END lock scope
		p.second();
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(mutex);
		memoize.erase(p.first);
		--job_cnt;
		if(job_cnt == 0){
			empty_cond.notify_all();
		}
		}//END lock scope
		consumer_cond.notify_one();
	}
}
//END dispatcher

//BEGIN conn_container
net::nstream_proactor::conn_container::conn_container():
	unused_conn_ID(0),
	last_time(std::time(NULL))
{}

void net::nstream_proactor::conn_container::add(const boost::shared_ptr<conn> & C)
{
	Socket.insert(std::make_pair(C->socket(), C));
	ID.insert(std::make_pair(C->ID(), C));
}

void net::nstream_proactor::conn_container::check_timeouts()
{
	if(last_time != std::time(NULL)){
		last_time = std::time(NULL);
		std::list<boost::uint64_t> timed_out;
		for(std::map<boost::uint64_t, boost::shared_ptr<conn> >::iterator
			it_cur = ID.begin(), it_end = ID.end(); it_cur != it_end; ++it_cur)
		{
			if(it_cur->second->timed_out()){
				timed_out.push_back(it_cur->second->ID());
			}
		}
		for(std::list<boost::uint64_t>::iterator it_cur = timed_out.begin(),
			it_end = timed_out.end(); it_cur != it_end; ++it_cur)
		{
			remove(*it_cur);
		}
	}
}

boost::uint64_t net::nstream_proactor::conn_container::new_conn_ID()
{
	return unused_conn_ID++;
}

std::set<int> net::nstream_proactor::conn_container::read_set()
{
	return _read_set;
}

void net::nstream_proactor::conn_container::monitor_read(const int socket_FD)
{
	_read_set.insert(socket_FD);
}

void net::nstream_proactor::conn_container::monitor_write(const int socket_FD)
{
	_write_set.insert(socket_FD);
}

void net::nstream_proactor::conn_container::perform_reads(std::set<int> read_set_in)
{
	for(std::set<int>::iterator it_cur = read_set_in.begin(),
		it_end = read_set_in.end(); it_cur != it_end; ++it_cur)
	{
		std::map<int, boost::shared_ptr<conn> >::iterator
			it = Socket.find(*it_cur);
		if(it != Socket.end()){
			it->second->read();
		}
	}
}

void net::nstream_proactor::conn_container::perform_writes(std::set<int> write_set_in)
{
	for(std::set<int>::iterator it_cur = write_set_in.begin(),
		it_end = write_set_in.end(); it_cur != it_end; ++it_cur)
	{
		std::map<int, boost::shared_ptr<conn> >::iterator
			it = Socket.find(*it_cur);
		if(it != Socket.end()){
			it->second->write();
		}
	}
}

void net::nstream_proactor::conn_container::remove(const boost::uint64_t conn_ID)
{
	std::map<boost::uint64_t, boost::shared_ptr<conn> >::iterator
		it = ID.find(conn_ID);
	if(it != ID.end()){
		_read_set.erase(it->second->socket());
		_write_set.erase(it->second->socket());
		Socket.erase(it->second->socket());
		ID.erase(it->second->ID());
	}
}

void net::nstream_proactor::conn_container::unmonitor_read(const int socket_FD)
{
	_read_set.erase(socket_FD);
}

void net::nstream_proactor::conn_container::unmonitor_write(const int socket_FD)
{
	_write_set.erase(socket_FD);
}

std::set<int> net::nstream_proactor::conn_container::write_set()
{
	return _write_set;
}
//END conn_container

//BEGIN conn_nstream
net::nstream_proactor::conn_nstream::conn_nstream(
	dispatcher & Dispatcher_in,
	conn_container & Conn_Container_in,
	boost::shared_ptr<nstream> N_in
):
	Dispatcher(Dispatcher_in),
	Conn_Container(Conn_Container_in),
	conn_ID(Conn_Container.new_conn_ID()),
	N(N_in)
{
	socket_FD = N->socket();
	if(N->is_open()){
		Conn_Container.monitor_read(socket_FD);
	}
}

boost::uint64_t net::nstream_proactor::conn_nstream::ID()
{
	return conn_ID;
}

void net::nstream_proactor::conn_nstream::read()
{

}

int net::nstream_proactor::conn_nstream::socket()
{
	return socket_FD;
}

bool net::nstream_proactor::conn_nstream::timed_out()
{
	return false;
}

void net::nstream_proactor::conn_nstream::write()
{

};
//END conn_nstream

//BEGIN conn_listener
net::nstream_proactor::conn_listener::conn_listener(
	dispatcher & Dispatcher_in,
	conn_container & Conn_Container_in,
	const endpoint & ep
):
	Dispatcher(Dispatcher_in),
	Conn_Container(Conn_Container_in),
	conn_ID(Conn_Container.new_conn_ID())
{
	Listener.open(ep);
	socket_FD = Listener.socket();
	if(Listener.is_open()){
		Conn_Container.monitor_read(socket_FD);
	}
}

boost::uint64_t net::nstream_proactor::conn_listener::ID()
{
	return conn_ID;
}

void net::nstream_proactor::conn_listener::read()
{
	while(boost::shared_ptr<nstream> N = Listener.accept()){
		LOG << N->remote_IP() << " " << N->remote_port() << " connected";
		boost::shared_ptr<conn_nstream> CN(new conn_nstream(Dispatcher,
			Conn_Container, N));
		Conn_Container.add(CN);
	}
}

int net::nstream_proactor::conn_listener::socket()
{
	return socket_FD;
}
//END conn_listener

net::nstream_proactor::nstream_proactor(
	const boost::function<void (connect_event)> & connect_call_back_in,
	const boost::function<void (disconnect_event)> & disconnect_call_back_in,
	const boost::function<void (recv_event)> & recv_call_back_in,
	const boost::function<void (send_event)> & send_call_back_in
):
	Dispatcher(
		connect_call_back_in,
		disconnect_call_back_in,
		recv_call_back_in,
		send_call_back_in
	),
	Internal_TP(1, 1024)
{
	Internal_TP.enqueue(boost::bind(&nstream_proactor::main_loop, this));
}

void net::nstream_proactor::listen(const endpoint & ep)
{
	Internal_TP.enqueue(boost::bind(&nstream_proactor::listen_relay, this, ep));
	Select.interrupt();
}

void net::nstream_proactor::listen_relay(const endpoint ep)
{
	boost::shared_ptr<conn_listener> CL(new conn_listener(Dispatcher,
		Conn_Container, ep));
	if(CL->socket() != -1){
		Conn_Container.add(CL);
	}else{
		LOG << "failed to start listener";
	}
}

void net::nstream_proactor::main_loop()
{
	std::set<int> read_set = Conn_Container.read_set();
	std::set<int> write_set = Conn_Container.write_set();
	Select(read_set, write_set, 1000);
	Conn_Container.perform_reads(read_set);
	Conn_Container.perform_writes(write_set);
	Conn_Container.check_timeouts();
	Internal_TP.enqueue(boost::bind(&nstream_proactor::main_loop, this));
}
