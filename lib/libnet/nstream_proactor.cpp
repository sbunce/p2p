#include <net/nstream_proactor.hpp>

//BEGIN events
net::nstream_proactor::conn_info::conn_info(
	const boost::uint64_t conn_ID_in,
	const dir_t dir_in,
	const tran_t tran_in,
	const boost::optional<endpoint> & local_ep_in,
	const boost::optional<endpoint> & remote_ep_in
):
	conn_ID(conn_ID_in),
	dir(dir_in),
	tran(tran_in),
	local_ep(local_ep_in),
	remote_ep(remote_ep_in)
{

}

net::nstream_proactor::connect_event::connect_event(
	const boost::shared_ptr<const conn_info> & info_in
):
	info(info_in)
{

}

net::nstream_proactor::disconnect_event::disconnect_event(
	const boost::shared_ptr<const conn_info> & info_in,
	const error_t error_in
):
	info(info_in),
	error(error_in)
{

}

net::nstream_proactor::recv_event::recv_event(
	const boost::shared_ptr<const conn_info> & info_in,
	const buffer & buf_in
):
	info(info_in),
	buf(buf_in)
{

}

net::nstream_proactor::send_event::send_event(
	const boost::shared_ptr<const conn_info> & info_in,
	const unsigned last_send_in,
	const unsigned send_buf_size_in
):
	info(info_in),
	last_send(last_send_in),
	send_buf_size(send_buf_size_in)
{

}
//END events

//BEGIN conn
net::nstream_proactor::conn::~conn()
{

}

void net::nstream_proactor::conn::schedule_send(const buffer & buf,
	const bool close_on_empty)
{

}

bool net::nstream_proactor::conn::timed_out()
{
	return false;
}

void net::nstream_proactor::conn::write()
{

}
//END conn

//BEGIN conn_container
net::nstream_proactor::conn_container::conn_container():
	unused_conn_ID(0),
	last_time(std::time(NULL))
{

}

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

void net::nstream_proactor::conn_container::perform_reads(
	const std::set<int> & read_set_in)
{
	for(std::set<int>::const_iterator it_cur = read_set_in.begin(),
		it_end = read_set_in.end(); it_cur != it_end; ++it_cur)
	{
		std::map<int, boost::shared_ptr<conn> >::iterator
			it = Socket.find(*it_cur);
		if(it != Socket.end()){
			it->second->read();
		}
	}
}

void net::nstream_proactor::conn_container::perform_writes(
	const std::set<int> & write_set_in)
{
	for(std::set<int>::const_iterator it_cur = write_set_in.begin(),
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

void net::nstream_proactor::conn_container::schedule_send(
	const boost::uint64_t conn_ID, const buffer & buf, const bool close_on_empty)
{
	std::map<boost::uint64_t, boost::shared_ptr<conn> >::iterator
		it = ID.find(conn_ID);
	if(it != ID.end()){
		it->second->schedule_send(buf, close_on_empty);
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
	const endpoint & ep
):
	Dispatcher(Dispatcher_in),
	Conn_Container(Conn_Container_in),
	N(new nstream()),
	conn_ID(Conn_Container.new_conn_ID()),
	close_on_empty(false),
	half_open(true),
	timeout(std::time(NULL) + connect_timeout),
	error(no_error)
{
	N->open_async(ep);
	socket_FD = N->socket();
	//socket either connected or failed to connect when writeable
	Conn_Container.monitor_write(socket_FD);
}

net::nstream_proactor::conn_nstream::conn_nstream(
	dispatcher & Dispatcher_in,
	conn_container & Conn_Container_in,
	boost::shared_ptr<nstream> N_in
):
	Dispatcher(Dispatcher_in),
	Conn_Container(Conn_Container_in),
	N(N_in),
	conn_ID(Conn_Container.new_conn_ID()),
	close_on_empty(false),
	half_open(false),
	timeout(std::time(NULL) + idle_timeout),
	error(no_error)
{
	socket_FD = N->socket();
	assert(N->is_open());
	info.reset(new conn_info(
		conn_ID,
		incoming_dir,
		nstream_tran,
		N->local_ep(),
		N->remote_ep()
	));
	Dispatcher.connect(connect_event(info));
	Conn_Container.monitor_read(socket_FD);
}

net::nstream_proactor::conn_nstream::~conn_nstream()
{
	Dispatcher.disconnect(disconnect_event(info, error));
}

boost::uint64_t net::nstream_proactor::conn_nstream::ID()
{
	return conn_ID;
}

void net::nstream_proactor::conn_nstream::read()
{
	buffer buf;
	int n_bytes = N->recv(buf);
	if(n_bytes <= 0){
		//assume connection reset (may not be)
		error = connection_reset_error;
		Conn_Container.remove(conn_ID);
	}else{
		Dispatcher.recv(recv_event(info, buf));
	}
}

void net::nstream_proactor::conn_nstream::schedule_send(const buffer & buf,
	const bool close_on_empty_in)
{
	if(close_on_empty_in){
		close_on_empty = true;
	}
	send_buf.append(buf);
	if(send_buf.empty() && close_on_empty){
		Conn_Container.remove(conn_ID);
	}else{
		Conn_Container.monitor_write(socket_FD);
	}
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
	if(half_open){
		info.reset(new conn_info(
			conn_ID,
			outgoing_dir,
			nstream_tran,
			N->local_ep(),
			N->remote_ep()
		));
		if(N->is_open_async()){
			half_open = false;
			Dispatcher.connect(connect_event(info));
			//send_buf always empty after connect
			Conn_Container.unmonitor_write(socket_FD);
			Conn_Container.monitor_read(socket_FD);
		}else{
			error = connect_error;
			Conn_Container.remove(conn_ID);
		}
	}else{
		int n_bytes = N->send(send_buf);
		if(send_buf.empty()){
			Conn_Container.unmonitor_write(socket_FD);
		}
		if(n_bytes <= 0){
			error = connection_reset_error;
			Conn_Container.remove(conn_ID);
		}else if(close_on_empty && send_buf.empty()){
			Conn_Container.remove(conn_ID);
		}else{
			Dispatcher.send(send_event(info, n_bytes, send_buf.size()));
		}
	}
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
	conn_ID(Conn_Container.new_conn_ID()),
	error(no_error)
{
	Listener.open(ep);
	Listener.set_non_blocking(true);
	socket_FD = Listener.socket();
	info.reset(new conn_info(
		conn_ID,
		outgoing_dir,
		nstream_listen_tran,
		ep
	));
	if(Listener.is_open()){
		Dispatcher.connect(connect_event(info));
		Conn_Container.monitor_read(socket_FD);
	}else{
		error = listen_error;
	}
}

net::nstream_proactor::conn_listener::~conn_listener()
{
	Dispatcher.disconnect(disconnect_event(info, error));
}

boost::uint64_t net::nstream_proactor::conn_listener::ID()
{
	return conn_ID;
}

boost::optional<net::endpoint> net::nstream_proactor::conn_listener::ep()
{
	return Listener.local_ep();
}

void net::nstream_proactor::conn_listener::read()
{
	while(boost::shared_ptr<nstream> N = Listener.accept()){
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
	producer_cnt(1),
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
	boost::mutex::scoped_lock lock(mutex);
	while(Job.size() >= max_buf){
		consumer_cond.wait(mutex);
	}
	boost::function<void ()> func = boost::bind(connect_call_back, CE);
	Job.push_back(std::make_pair(CE.info->conn_ID, func));
	++producer_cnt;
	++job_cnt;
	producer_cond.notify_one();
}

void net::nstream_proactor::dispatcher::disconnect(const disconnect_event & DE)
{
	boost::mutex::scoped_lock lock(mutex);
	while(Job.size() >= max_buf){
		consumer_cond.wait(mutex);
	}
	boost::function<void ()> func = boost::bind(disconnect_call_back, DE);
	Job.push_back(std::make_pair(DE.info->conn_ID, func));
	++producer_cnt;
	++job_cnt;
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
	boost::mutex::scoped_lock lock(mutex);
	while(Job.size() >= max_buf){
		consumer_cond.wait(mutex);
	}
	boost::function<void ()> func = boost::bind(recv_call_back, RE);
	Job.push_back(std::make_pair(RE.info->conn_ID, func));
	++producer_cnt;
	++job_cnt;
	producer_cond.notify_one();
}

void net::nstream_proactor::dispatcher::send(const send_event & SE)
{
	boost::mutex::scoped_lock lock(mutex);
	while(Job.size() >= max_buf){
		consumer_cond.wait(mutex);
	}
	boost::function<void ()> func = boost::bind(send_call_back, SE);
	Job.push_back(std::make_pair(SE.info->conn_ID, func));
	++producer_cnt;
	++job_cnt;
	producer_cond.notify_one();
}

void net::nstream_proactor::dispatcher::dispatch()
{
	//when no job to run we wait until producer_cnt greater than this
	boost::uint64_t wait_until = 0;
	while(true){
		std::pair<boost::uint64_t, boost::function<void ()> > p;
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(mutex);
		while(Job.empty() || wait_until > producer_cnt){
			producer_cond.wait(mutex);
		}
		for(std::list<std::pair<boost::uint64_t, boost::function<void ()> > >::iterator
			it_cur = Job.begin(), it_end = Job.end(); it_cur != it_end; ++it_cur)
		{
			if(memoize.insert(it_cur->first).second){
				p = *it_cur;
				Job.erase(it_cur);
				break;
			}
		}
		if(!p.second){
			//no job we can currently run, wait until a job added to check again
			wait_until = producer_cnt + 1;
			continue;
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

void net::nstream_proactor::connect(const endpoint & ep)
{
	Internal_TP.enqueue(boost::bind(&nstream_proactor::connect_relay, this, ep));
	Select.interrupt();
}

void net::nstream_proactor::connect_relay(const endpoint & ep)
{
	boost::shared_ptr<conn_nstream> CL(new conn_nstream(Dispatcher,
		Conn_Container, ep));
	if(CL->socket() != -1){
		Conn_Container.add(CL);
	}
}

void net::nstream_proactor::disconnect(const boost::uint64_t conn_ID)
{
	Internal_TP.enqueue(boost::bind(&nstream_proactor::disconnect_relay, this, conn_ID));
	Select.interrupt();
}

void net::nstream_proactor::disconnect_relay(const boost::uint64_t conn_ID)
{
	Conn_Container.remove(conn_ID);
}

channel::future<std::string> net::nstream_proactor::listen(const endpoint & ep)
{
	std::pair<channel::promise<std::string>, channel::future<std::string> >
		p = channel::make_future<std::string>();
	Internal_TP.enqueue(boost::bind(&nstream_proactor::listen_relay, this, ep,
		p.first));
	Select.interrupt();
	return p.second;
}

void net::nstream_proactor::listen_relay(const endpoint ep,
	channel::promise<std::string> promise)
{
	boost::shared_ptr<conn_listener> CL(new conn_listener(Dispatcher,
		Conn_Container, ep));
	if(CL->socket() != -1){
		Conn_Container.add(CL);
	}
	promise = (CL->ep() ? CL->ep()->port() : "");
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

void net::nstream_proactor::send(const boost::uint64_t conn_ID,
	const buffer & buf, const bool close_on_empty)
{
	Internal_TP.enqueue(boost::bind(&nstream_proactor::send_relay, this, conn_ID,
		buf, close_on_empty));
	Select.interrupt();
}

void net::nstream_proactor::send_relay(const boost::uint64_t conn_ID,
	const buffer buf, const bool close_on_empty)
{
	Conn_Container.schedule_send(conn_ID, buf, close_on_empty);
}
