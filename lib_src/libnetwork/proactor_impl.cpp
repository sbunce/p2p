#include "proactor_impl.hpp"

network::proactor_impl::proactor_impl(
	const boost::function<void (connection_info &)> & connect_call_back,
	const boost::function<void (connection_info &)> & disconnect_call_back
):
	Dispatcher(connect_call_back, disconnect_call_back),
	incoming_connection_limit(FD_SETSIZE / 2),
	outgoing_connection_limit(FD_SETSIZE / 2),
	incoming_connections(0),
	outgoing_connections(0)
{

}

void network::proactor_impl::add_socket(std::pair<int, boost::shared_ptr<connection> > P)
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

void network::proactor_impl::adjust_connection_limits(const unsigned incoming_limit,
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

void network::proactor_impl::append_send_buf(const int connection_ID,
	boost::shared_ptr<buffer> B)
{
	std::pair<int, boost::shared_ptr<connection> > P = lookup_ID(connection_ID);
	if(P.second){
		P.second->send_buf.append(*B);
		write_FDS.insert(P.second->socket());
	}
}

void network::proactor_impl::check_timeouts()
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
		if(it_cur->second->timed_out()){
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

void network::proactor_impl::connect(const std::string & host, const std::string & port)
{
	boost::mutex::scoped_lock lock(network_thread_call_mutex);
	network_thread_call.push_back(boost::bind(&proactor_impl::resolve_relay, this,
		host, port));
}

void network::proactor_impl::disconnect(const int connection_ID)
{
	boost::mutex::scoped_lock lock(network_thread_call_mutex);
	network_thread_call.push_back(boost::bind(&proactor_impl::disconnect, this,
		connection_ID, false));
	Select.interrupt();
}

void network::proactor_impl::disconnect(const int connection_ID, const bool on_empty)
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

void network::proactor_impl::disconnect_on_empty(const int connection_ID)
{
	boost::mutex::scoped_lock lock(network_thread_call_mutex);
	network_thread_call.push_back(boost::bind(&proactor_impl::disconnect, this,
		connection_ID, true));
	Select.interrupt();
}

unsigned network::proactor_impl::download_rate()
{
	return Rate_Limit.download();
}

unsigned network::proactor_impl::get_max_download_rate()
{
	return Rate_Limit.max_download();
}

unsigned network::proactor_impl::get_max_upload_rate()
{
	return Rate_Limit.max_upload();
}

void network::proactor_impl::handle_async_connection(
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
		//async connection failed, try next endpoint
		remove_socket(P.first);
		if(P.second->open_async()){
			//async connection in progress
			P.first = P.second->socket();
			add_socket(P);
		}else{
			//failed to allocate socket
			Dispatcher.disconnect(P.second->CI);
		}
	}
}

std::string network::proactor_impl::listen_port()
{
	/*
	It is thread safe to return this because it is connected before the network
	thread is started and won't disconnect while the network thread is active.
	*/
	return Listener->port();
}

std::pair<int, boost::shared_ptr<network::connection> >
	network::proactor_impl::lookup_socket(const int socket_FD)
{
	std::map<int, boost::shared_ptr<connection> >::iterator
		iter = Socket.find(socket_FD);
	if(iter == Socket.end()){
		return std::pair<int, boost::shared_ptr<connection> >();
	}else{
		return *iter;
	}
}

std::pair<int, boost::shared_ptr<network::connection> >
	network::proactor_impl::lookup_ID(const int connection_ID)
{
	std::map<int, boost::shared_ptr<connection> >::iterator
		iter = ID.find(connection_ID);
	if(iter == ID.end()){
		return std::pair<int, boost::shared_ptr<connection> >();
	}else{
		return *iter;
	}
}

void network::proactor_impl::network_loop()
{
	std::time_t last_loop_time(std::time(NULL));
	std::set<int> tmp_read_FDS, tmp_write_FDS;
	while(true){
		boost::this_thread::interruption_point();
		if(last_loop_time != std::time(NULL)){
			//stuff to run only once per second
			check_timeouts();
			last_loop_time = std::time(NULL);
		}
		process_network_thread_call();

		//only check for reads and writes when there is available download/upload
		if(Rate_Limit.available_download() == 0){
			tmp_read_FDS.clear();
		}else{
			tmp_read_FDS = read_FDS;
		}
		if(Rate_Limit.available_upload() == 0){
			tmp_write_FDS.clear();
		}else{
			tmp_write_FDS = write_FDS;
		}

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
					N->close();
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
	}
}

void network::proactor_impl::process_network_thread_call()
{
	while(true){
		boost::function<void ()> tmp;
		{//begin lock scope
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		if(network_thread_call.empty()){
			break;
		}
		tmp = network_thread_call.front();
		network_thread_call.pop_front();
		}//end lock scope
		tmp();
	}
}

void network::proactor_impl::remove_socket(const int socket_FD)
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

void network::proactor_impl::resolve_relay(const std::string host, const std::string port)
{
	if(outgoing_connections < outgoing_connection_limit){
		++outgoing_connections;
		Thread_Pool.enqueue(boost::bind(&proactor_impl::resolve, this, host, port));
	}else{
		//connection limit reached
		boost::shared_ptr<connection_info> CI(new connection_info(
			ID_Manager.allocate(),
			host,
			"",
			port,
			outgoing
		));
		ID_Manager.deallocate(CI->connection_ID);
		Dispatcher.disconnect(CI);
	}
}

void network::proactor_impl::resolve(const std::string & host, const std::string & port)
{
LOG;
	boost::shared_ptr<connection> Connection(new connection(ID_Manager, host, port));
LOG;
	if(Connection->open_async()){
LOG;
		//in progress of connecting
		std::pair<int, boost::shared_ptr<connection> > P(Connection->socket(), Connection);
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		network_thread_call.push_back(boost::bind(&proactor_impl::add_socket, this, P));
		}//END lock scope
	}else{
LOG;
		//failed to resolve or failed to allocate socket
		Dispatcher.disconnect(Connection->CI);
	}
LOG;
	Select.interrupt();
}

void network::proactor_impl::send(const int connection_ID, buffer & send_buf)
{
	if(!send_buf.empty()){
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		boost::shared_ptr<buffer> buf(new buffer());
		buf->swap(send_buf); //swap buffers to avoid copy
		network_thread_call.push_back(boost::bind(&proactor_impl::append_send_buf, this,
			connection_ID, buf));
		Select.interrupt();
	}
}

void network::proactor_impl::set_connection_limit(const unsigned incoming_limit,
	const unsigned outgoing_limit)
{
	assert(incoming_limit + outgoing_limit <= FD_SETSIZE);
	boost::mutex::scoped_lock lock(network_thread_call_mutex);
	network_thread_call.push_back(boost::bind(&proactor_impl::adjust_connection_limits,
		this, incoming_limit, outgoing_limit));
}

void network::proactor_impl::set_max_download_rate(const unsigned rate)
{
	Rate_Limit.max_download(rate);
}

void network::proactor_impl::set_max_upload_rate(const unsigned rate)
{
	Rate_Limit.max_upload(rate);
}

void network::proactor_impl::start(boost::shared_ptr<listener> Listener_in)
{
	boost::recursive_mutex::scoped_lock lock(start_stop_mutex);
	if(network_thread.get_id() != boost::thread::id()){
		LOG << "error, network_thread already started";
		exit(1);
	}
	if(Listener_in){
		assert(Listener_in->is_open());
		Listener = Listener_in;
		Listener->set_non_blocking();
		add_socket(std::make_pair(Listener->socket(), boost::shared_ptr<connection>()));
	}
	network_thread = boost::thread(boost::bind(&proactor_impl::network_loop, this));
	Dispatcher.start();
	Thread_Pool.start();
}

void network::proactor_impl::stop()
{
	boost::recursive_mutex::scoped_lock lock(start_stop_mutex);

	//cancel all resolve jobs
	Thread_Pool.stop_clear_join();

	//stop networking thread
	network_thread.interrupt();
	Select.interrupt();
	network_thread.join();

	//Note: At this point we can modify private data since network_thread stopped

	//disconnect everything
	for(std::map<int, boost::shared_ptr<connection> >::iterator it_cur = Socket.begin(),
		it_end = Socket.end(); it_cur != it_end; ++it_cur)
	{
		if(it_cur->second->CI){
			Dispatcher.disconnect(it_cur->second->CI);
			it_cur->second->N->close();
		}
	}
	Listener = boost::shared_ptr<listener>();

	//reset proactor_impl state
	read_FDS.clear();
	write_FDS.clear();
	Socket.clear();
	ID.clear();
	incoming_connections = 0;
	outgoing_connections = 0;
	network_thread_call.clear();

	//stop dispatcher and wait for dispatcher call backs to finish
	Dispatcher.stop_join();
}

unsigned network::proactor_impl::upload_rate()
{
	return Rate_Limit.upload();
}
