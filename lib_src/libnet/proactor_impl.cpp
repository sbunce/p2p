#include "proactor_impl.hpp"

net::proactor_impl::proactor_impl(
	const boost::function<void (proactor::connection_info &)> & connect_call_back,
	const boost::function<void (proactor::connection_info &)> & disconnect_call_back
):
	Dispatcher(connect_call_back, disconnect_call_back),
	network_loop_time(std::time(NULL)),
	incoming_connection_limit(512),
	outgoing_connection_limit(512),
	incoming_connections(0),
	outgoing_connections(0),
	Thread_Pool(this)
{
	Thread_Pool.stop();
}

void net::proactor_impl::add_socket(std::pair<int, boost::shared_ptr<connection> > P)
{
	assert(P.first != -1);
	if(P.second){
		assert(P.second->N);
		assert(P.second->CI);
		if(P.second->half_open()){
			//async connection in progress
			write_FDS.insert(P.first);
		}else{
			read_FDS.insert(P.first);
		}
		std::pair<std::map<int, boost::shared_ptr<connection> >::iterator, bool>
			ret = Socket.insert(P);
		assert(ret.second);
		P.first = P.second->connection_ID;
		ret = ID.insert(P);
		assert(ret.second);
	}else{
		//the listen socket doesn't have connection associated with it
		read_FDS.insert(P.first);
	}
}

void net::proactor_impl::adjust_connection_limits(const unsigned incoming_limit,
	const unsigned outgoing_limit)
{
	incoming_connection_limit = incoming_limit;
	outgoing_connection_limit = outgoing_limit;

	//determine if any incoming need to be disconnected
	if(incoming_connections > incoming_connection_limit){
		//disconnect random sockets until limit met
		std::vector<int> incoming_socket;
		for(std::map<int, boost::shared_ptr<connection> >::iterator it_cur = Socket.begin(),
			it_end = Socket.end(); it_cur != it_end; ++it_cur)
		{
			if(it_cur->second->CI->direction == incoming){
				incoming_socket.push_back(it_cur->first);
			}
		}
		assert(incoming_socket.size() == incoming_connections);
		std::random_shuffle(incoming_socket.begin(), incoming_socket.end());
		while(incoming_socket.size() > incoming_limit){
			std::pair<int, boost::shared_ptr<connection> > P = lookup_socket(incoming_socket.back());
			Dispatcher.disconnect(P.second->CI);
			remove_socket(incoming_socket.back());
			incoming_socket.pop_back();
		}
	}

	//determine if any outgoing need to be disconnected
	if(outgoing_connections > outgoing_connection_limit){
		//disconnect random sockets until limit met
		std::vector<int> outgoing_socket;
		for(std::map<int, boost::shared_ptr<connection> >::iterator it_cur = Socket.begin(),
			it_end = Socket.end(); it_cur != it_end; ++it_cur)
		{
			if(it_cur->second->CI->direction == outgoing){
				outgoing_socket.push_back(it_cur->first);
			}
		}
		assert(outgoing_socket.size() == outgoing_connections);
		std::random_shuffle(outgoing_socket.begin(), outgoing_socket.end());
		while(outgoing_socket.size() > outgoing_limit){
			std::pair<int, boost::shared_ptr<connection> > P = lookup_socket(outgoing_socket.back());
			Dispatcher.disconnect(P.second->CI);
			remove_socket(outgoing_socket.back());
			outgoing_socket.pop_back();
		}
	}
}

void net::proactor_impl::append_send_buf(const int connection_ID,
	boost::shared_ptr<buffer> B)
{
	std::pair<int, boost::shared_ptr<connection> > P = lookup_ID(connection_ID);
	if(P.second){
		P.second->send_buf.append(*B);
		write_FDS.insert(P.second->socket());
	}
}

void net::proactor_impl::check_timeouts()
{
	/*
	We save the elements we want to erase because calling remove_socket
	invalidates iterators for the Socket container.
	*/
	std::vector<std::pair<int, boost::shared_ptr<connection> > > timed_out;
	for(std::map<int, boost::shared_ptr<connection> >::iterator
		it_cur = Socket.begin(), it_end = Socket.end(); it_cur != it_end;
		++it_cur)
	{
		if(it_cur->second->timeout()){
			timed_out.push_back(*it_cur);
		}
	}

	for(std::vector<std::pair<int, boost::shared_ptr<connection> > >::iterator
		it_cur = timed_out.begin(), it_end = timed_out.end();
		it_cur != it_end; ++it_cur)
	{
		LOG << "timed out " << it_cur->second->N->remote_IP();
		remove_socket(it_cur->first);
		Dispatcher.disconnect(it_cur->second->CI);
	}
}

void net::proactor_impl::connect(const endpoint & ep)
{
	boost::mutex::scoped_lock lock(relay_job_mutex);
	relay_job.push_back(boost::bind(&proactor_impl::connect_relay, this, ep));
}

void net::proactor_impl::connect_relay(const endpoint ep)
{
	boost::shared_ptr<connection> Connection(new connection(ID_Manager, ep));
	if(Connection->socket() != -1){
		//in progress of connecting
		std::pair<int, boost::shared_ptr<connection> > P(Connection->socket(), Connection);
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(relay_job_mutex);
		relay_job.push_back(boost::bind(&proactor_impl::add_socket, this, P));
		}//END lock scope
		++outgoing_connections;
	}else{
		//failed to resolve or failed to allocate socket
		Dispatcher.disconnect(Connection->CI);
	}
	Select.interrupt();
}

void net::proactor_impl::disconnect(const int connection_ID)
{
	boost::mutex::scoped_lock lock(relay_job_mutex);
	relay_job.push_back(boost::bind(&proactor_impl::disconnect, this,
		connection_ID, false));
	Select.interrupt();
}

void net::proactor_impl::disconnect(const int connection_ID, const bool on_empty)
{
	std::pair<int, boost::shared_ptr<connection> > P = lookup_ID(connection_ID);
	if(P.second){
		if(on_empty){
			//disconnect when send_buf empty
			if(P.second->send_buf.empty()){
				remove_socket(P.second->socket());
				Dispatcher.disconnect(P.second->CI);
			}else{
				P.second->disc_on_empty = true;
			}
		}else{
			//disconnect regardless of whether send_buf empty
			remove_socket(P.second->socket());
			Dispatcher.disconnect(P.second->CI);
		}
	}
}

void net::proactor_impl::disconnect_on_empty(const int connection_ID)
{
	boost::mutex::scoped_lock lock(relay_job_mutex);
	relay_job.push_back(boost::bind(&proactor_impl::disconnect, this,
		connection_ID, true));
	Select.interrupt();
}

unsigned net::proactor_impl::download_rate()
{
	return Rate_Limit.download();
}

void net::proactor_impl::handle_async_connection(
	std::pair<int, boost::shared_ptr<connection> > P)
{
	assert(P.first != -1);
	P.second->touch();
	if(P.second->is_open()){
		//async connection suceeded
		read_FDS.insert(P.first); //monitor socket for incoming data
		write_FDS.erase(P.first); //send_buf will be empty after connect
		Dispatcher.connect(P.second->CI);
	}else{
		//async connection failed
		remove_socket(P.first);
		Dispatcher.disconnect(P.second->CI);
	}
}

std::pair<int, boost::shared_ptr<net::connection> >
	net::proactor_impl::lookup_socket(const int socket_FD)
{
	std::map<int, boost::shared_ptr<connection> >::iterator
		iter = Socket.find(socket_FD);
	if(iter == Socket.end()){
		return std::pair<int, boost::shared_ptr<connection> >();
	}else{
		return *iter;
	}
}

std::pair<int, boost::shared_ptr<net::connection> >
	net::proactor_impl::lookup_ID(const int connection_ID)
{
	std::map<int, boost::shared_ptr<connection> >::iterator
		iter = ID.find(connection_ID);
	if(iter == ID.end()){
		return std::pair<int, boost::shared_ptr<connection> >();
	}else{
		return *iter;
	}
}

void net::proactor_impl::network_loop()
{
	if(network_loop_time != std::time(NULL)){
		//stuff to run only once per second
		check_timeouts();
		network_loop_time = std::time(NULL);
	}
	process_relay_job();

	//only check for reads and writes when there is available download/upload
	std::set<int> tmp_read_FDS, tmp_write_FDS;
	if(Rate_Limit.available_download() > 0){
		tmp_read_FDS = read_FDS;
	}
	if(Rate_Limit.available_upload() > 0){
		tmp_write_FDS = write_FDS;
	}

/* DEBUG
When network test harness is created there will be a 'fake' Select here which
doesn't block.
*/
	Select(tmp_read_FDS, tmp_write_FDS, 10);

	if(Listener && Listener->is_open() &&
		tmp_read_FDS.find(Listener->socket()) != tmp_read_FDS.end())
	{
		//handle incoming connections
		while(boost::shared_ptr<nstream> N = Listener->accept()){
			std::pair<int, boost::shared_ptr<connection> > P(N->socket(),
				boost::shared_ptr<connection>(new connection(ID_Manager, N)));
			if(incoming_connections < incoming_connection_limit){
				++incoming_connections;
				add_socket(P);
				Dispatcher.connect(P.second->CI);
			}else{
				Dispatcher.disconnect(P.second->CI);
			}
		}
		tmp_read_FDS.erase(Listener->socket());
	}

	//handle all writes
	while(!tmp_write_FDS.empty()){
		std::pair<int, boost::shared_ptr<connection> > P = lookup_socket(*tmp_write_FDS.begin());
		P.second->touch();
		if(P.second->half_open()){
			handle_async_connection(P);
		}else{
			//n_bytes initially set to max send, then set to how much sent
			int n_bytes = Rate_Limit.available_upload(tmp_write_FDS.size());
			if(n_bytes != 0){
				n_bytes = P.second->N->send(P.second->send_buf, n_bytes);
				if(n_bytes == -1 || n_bytes == 0){
					remove_socket(P.first);
					Dispatcher.disconnect(P.second->CI);
				}else{
					Rate_Limit.add_upload(n_bytes);
					if(P.second->send_buf.empty()){
						if(P.second->disc_on_empty){
							remove_socket(P.first);
							Dispatcher.send(P.second->CI, n_bytes, P.second->send_buf.size());
							Dispatcher.disconnect(P.second->CI);
						}else{
							write_FDS.erase(P.first);
							Dispatcher.send(P.second->CI, n_bytes, P.second->send_buf.size());
						}
					}else{
						Dispatcher.send(P.second->CI, n_bytes, P.second->send_buf.size());
					}
				}
			}
		}
		tmp_write_FDS.erase(tmp_write_FDS.begin());
	}

	//handle all reads
	while(!tmp_read_FDS.empty()){
		std::pair<int, boost::shared_ptr<connection> > P = lookup_socket(*tmp_read_FDS.begin());
		if(P.second){
			P.second->touch();
			//n_bytes initially set to max read, then set to how much read
			int n_bytes = Rate_Limit.available_download(tmp_read_FDS.size());
			if(n_bytes == 0){
				//maxed out what can be read
				break;
			}
			boost::shared_ptr<buffer> recv_buf(new buffer());
			n_bytes = P.second->N->recv(*recv_buf, n_bytes);
			if(n_bytes == -1 || n_bytes == 0){
				remove_socket(P.first);
				Dispatcher.disconnect(P.second->CI);
			}else{
				Rate_Limit.add_download(n_bytes);
				Dispatcher.recv(P.second->CI, recv_buf);
			}
		}
		tmp_read_FDS.erase(tmp_read_FDS.begin());
	}
	Thread_Pool.enqueue(boost::bind(&net::proactor_impl::network_loop, this));
}

void net::proactor_impl::process_relay_job()
{
	while(true){
		boost::function<void ()> tmp;
		{//begin lock scope
		boost::mutex::scoped_lock lock(relay_job_mutex);
		if(relay_job.empty()){
			break;
		}
		tmp = relay_job.front();
		relay_job.pop_front();
		}//end lock scope
		tmp();
	}
}

void net::proactor_impl::remove_socket(const int socket_FD)
{
	assert(socket_FD != -1);
	read_FDS.erase(socket_FD);
	write_FDS.erase(socket_FD);
	std::pair<int, boost::shared_ptr<connection> > P = lookup_socket(socket_FD);
	if(P.second){
		if(P.second->CI->direction == incoming){
			--incoming_connections;
		}else{
			--outgoing_connections;
		}
		Socket.erase(socket_FD);
		ID.erase(P.second->connection_ID);
	}
}

void net::proactor_impl::send(const int connection_ID, buffer & send_buf)
{
	if(!send_buf.empty()){
		boost::mutex::scoped_lock lock(relay_job_mutex);
		boost::shared_ptr<buffer> buf(new buffer());
		buf->swap(send_buf); //swap buffers to avoid copy
		relay_job.push_back(boost::bind(&proactor_impl::append_send_buf, this,
			connection_ID, buf));
		Select.interrupt();
	}
}

void net::proactor_impl::set_connection_limit(const unsigned incoming_limit,
	const unsigned outgoing_limit)
{
	assert(incoming_limit + outgoing_limit <= 1024);
	boost::mutex::scoped_lock lock(relay_job_mutex);
	relay_job.push_back(boost::bind(&proactor_impl::adjust_connection_limits,
		this, incoming_limit, outgoing_limit));
}

void net::proactor_impl::set_max_download_rate(const unsigned rate)
{
	Rate_Limit.set_max_download(rate);
}

void net::proactor_impl::set_max_upload_rate(const unsigned rate)
{
	Rate_Limit.set_max_upload(rate);
}

void net::proactor_impl::start(boost::shared_ptr<listener> Listener_in)
{
	boost::recursive_mutex::scoped_lock lock(start_stop_mutex);
	if(Thread_Pool.is_started()){
		LOG << "proactor already started";
		exit(1);
	}
	if(Listener_in){
		assert(Listener_in->is_open());
		Listener = Listener_in;
		if(!Listener->set_non_blocking(true)){
			LOG << "failed to set listener to non-blocking\n";
			exit(1);
		}
		add_socket(std::make_pair(Listener->socket(), boost::shared_ptr<connection>()));
	}
	Thread_Pool.start();
	Thread_Pool.enqueue(boost::bind(&proactor_impl::network_loop, this));
}

void net::proactor_impl::stop()
{
	boost::recursive_mutex::scoped_lock lock(start_stop_mutex);

	//stop networking thread
	Thread_Pool.stop();
	Select.interrupt();
	Thread_Pool.join();

	//note: At this point we can modify private data since network thread stopped

	//disconnect everything
	for(std::map<int, boost::shared_ptr<connection> >::iterator it_cur = Socket.begin(),
		it_end = Socket.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second->CI){
			Dispatcher.disconnect(it_cur->second->CI);
		}
	}
	if(!Listener->set_non_blocking(false)){
		LOG << "failed to set listener to blocking\n";
		exit(1);
	}
	Listener.reset();

	//reset proactor_impl state
	read_FDS.clear();
	write_FDS.clear();
	Socket.clear();
	ID.clear();
	incoming_connections = 0;
	outgoing_connections = 0;
	relay_job.clear();

	//wait for all dispatches to finish
	Dispatcher.join();
}

unsigned net::proactor_impl::upload_rate()
{
	return Rate_Limit.upload();
}
