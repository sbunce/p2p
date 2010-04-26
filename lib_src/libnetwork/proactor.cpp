#include <network/network.hpp>

//BEGIN ID_manager
network::proactor::ID_manager::ID_manager():
	highest_allocated(0)
{

}

int network::proactor::ID_manager::allocate()
{
	boost::mutex::scoped_lock lock(Mutex);
	while(true){
		++highest_allocated;
		if(allocated.find(highest_allocated) == allocated.end()){
			allocated.insert(highest_allocated);
			return highest_allocated;
		}
	}
}

void network::proactor::ID_manager::deallocate(int connection_ID)
{
	boost::mutex::scoped_lock lock(Mutex);
	if(allocated.erase(connection_ID) != 1){
		LOG << "double free of connection_ID";
		exit(1);
	}
}
//END ID_manager

//BEGIN init
network::proactor::init::init()
{
	network::start();
}

network::proactor::init::~init()
{
	network::stop();
}
//END init

//BEGIN connection
network::proactor::connection::connection(
	ID_manager & ID_Manager_in,
	const std::string & host_in,
	const std::string & port_in
):
	connected(false),
	ID_Manager(ID_Manager_in),
	last_seen(std::time(NULL)),
	disc_on_empty(false),
	N(new nstream()),
	host(host_in),
	port(port_in),
	connection_ID(ID_Manager.allocate())
{
	E = get_endpoint(host, port, tcp);
	if(E.empty()){
		CI = boost::shared_ptr<connection_info>(new connection_info(connection_ID,
		host, "", port, incoming));
	}
}

network::proactor::connection::connection(
	ID_manager & ID_Manager_in,
	const boost::shared_ptr<nstream> & N_in
):
	connected(true),
	ID_Manager(ID_Manager_in),
	last_seen(std::time(NULL)),
	socket_FD(N_in->socket()),
	disc_on_empty(false),
	N(N_in),
	host(""),
	port(N_in->remote_port()),
	connection_ID(ID_Manager.allocate())
{
	CI = boost::shared_ptr<connection_info>(new connection_info(connection_ID,
		"", N->remote_IP(), N->remote_port(), incoming));
}

network::proactor::connection::~connection()
{
	ID_Manager.deallocate(connection_ID);
}

bool network::proactor::connection::half_open()
{
	return !connected;
}

bool network::proactor::connection::is_open()
{
	if(N->is_open_async()){
		connected = true;
		return true;
	}
	return false;
}

bool network::proactor::connection::open_async()
{
	//assert this connection is outgoing
	assert(!host.empty());
	if(E.empty()){
		//no more endpoints to try
		return false;
	}
	N->open_async(*E.begin());
	socket_FD = N->socket();
	CI = boost::shared_ptr<connection_info>(new connection_info(
		connection_ID, host, E.begin()->IP(), port, outgoing));
	E.erase(E.begin());
	return socket_FD != -1;
}

int network::proactor::connection::socket()
{
	return socket_FD;
}

bool network::proactor::connection::timed_out()
{
	return std::time(NULL) - last_seen > idle_timeout;
}

void network::proactor::connection::touch()
{
	last_seen = std::time(NULL);
}
//END connection

network::proactor::proactor(
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

void network::proactor::add_socket(std::pair<int, boost::shared_ptr<connection> > P)
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

void network::proactor::adjust_connection_limits(const unsigned incoming_limit,
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

void network::proactor::append_send_buf(const int connection_ID,
	boost::shared_ptr<buffer> B)
{
	std::pair<int, boost::shared_ptr<connection> > P = lookup_ID(connection_ID);
	if(P.second){
		P.second->send_buf.append(*B);
		write_FDS.insert(P.second->socket());
	}
}

void network::proactor::check_timeouts()
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

void network::proactor::connect(const std::string & host, const std::string & port)
{
	boost::mutex::scoped_lock lock(network_thread_call_mutex);
	network_thread_call.push_back(boost::bind(&proactor::resolve_relay, this,
		host, port));
}

void network::proactor::disconnect(const int connection_ID)
{
	boost::mutex::scoped_lock lock(network_thread_call_mutex);
	network_thread_call.push_back(boost::bind(&proactor::disconnect, this,
		connection_ID, false));
	Select.interrupt();
}

void network::proactor::disconnect(const int connection_ID, const bool on_empty)
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

void network::proactor::disconnect_on_empty(const int connection_ID)
{
	boost::mutex::scoped_lock lock(network_thread_call_mutex);
	network_thread_call.push_back(boost::bind(&proactor::disconnect, this,
		connection_ID, true));
	Select.interrupt();
}

unsigned network::proactor::download_rate()
{
	return Rate_Limit.download();
}

void network::proactor::handle_async_connection(
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

std::string network::proactor::listen_port()
{
	return Listener.port();
}

std::pair<int, boost::shared_ptr<network::proactor::connection> >
	network::proactor::lookup_socket(const int socket_FD)
{
	std::map<int, boost::shared_ptr<connection> >::iterator
		iter = Socket.find(socket_FD);
	if(iter == Socket.end()){
		return std::pair<int, boost::shared_ptr<connection> >();
	}else{
		return *iter;
	}
}

std::pair<int, boost::shared_ptr<network::proactor::connection> >
	network::proactor::lookup_ID(const int connection_ID)
{
	std::map<int, boost::shared_ptr<connection> >::iterator
		iter = ID.find(connection_ID);
	if(iter == ID.end()){
		return std::pair<int, boost::shared_ptr<connection> >();
	}else{
		return *iter;
	}
}

unsigned network::proactor::max_download_rate()
{
	return Rate_Limit.max_download();
}

void network::proactor::max_download_rate(const unsigned rate)
{
	Rate_Limit.max_download(rate);
}

unsigned network::proactor::max_upload_rate()
{
	return Rate_Limit.max_upload();
}

void network::proactor::max_upload_rate(const unsigned rate)
{
	Rate_Limit.max_upload(rate);
}

void network::proactor::network_loop()
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

		if(Listener.is_open() && tmp_read_FDS.find(Listener.socket()) != tmp_read_FDS.end()){
			//handle incoming connections
			while(boost::shared_ptr<nstream> N = Listener.accept()){
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
			tmp_read_FDS.erase(Listener.socket());
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

void network::proactor::process_network_thread_call()
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

void network::proactor::remove_socket(const int socket_FD)
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

void network::proactor::resolve_relay(const std::string host, const std::string port)
{
	if(outgoing_connections < outgoing_connection_limit){
		++outgoing_connections;
		Thread_Pool.enqueue(boost::bind(&proactor::resolve, this, host, port));
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

void network::proactor::resolve(const std::string & host, const std::string & port)
{
	boost::shared_ptr<connection> Connection(new connection(ID_Manager, host, port));
	if(Connection->open_async()){
		//in progress of connecting
		std::pair<int, boost::shared_ptr<connection> > P(Connection->socket(), Connection);
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		network_thread_call.push_back(boost::bind(&proactor::add_socket, this, P));
		}//END lock scope
	}else{
		//failed to resolve or failed to allocate socket
		Dispatcher.disconnect(Connection->CI);
	}
	Select.interrupt();
}

void network::proactor::send(const int connection_ID, buffer & send_buf)
{
	if(!send_buf.empty()){
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		boost::shared_ptr<buffer> buf(new buffer());
		buf->swap(send_buf); //swap buffers to avoid copy
		network_thread_call.push_back(boost::bind(&proactor::append_send_buf, this,
			connection_ID, buf));
		Select.interrupt();
	}
}

void network::proactor::set_connection_limit(const unsigned incoming_limit,
	const unsigned outgoing_limit)
{
	assert(incoming_limit + outgoing_limit <= FD_SETSIZE);
	boost::mutex::scoped_lock lock(network_thread_call_mutex);
	network_thread_call.push_back(boost::bind(&proactor::adjust_connection_limits,
		this, incoming_limit, outgoing_limit));
}

bool network::proactor::start()
{
	network_thread = boost::thread(boost::bind(&proactor::network_loop, this));
	Dispatcher.start();
	return true;
}

bool network::proactor::start(const endpoint & E)
{
	assert(E.type() == tcp);
	Listener.open(E);
	if(!Listener.is_open()){
		LOG << "failed to start listener";
		return false;
	}
	Listener.set_non_blocking();
	add_socket(std::make_pair(Listener.socket(), boost::shared_ptr<connection>()));
	return start();
}

void network::proactor::stop()
{
	Thread_Pool.clear_stop();
	Dispatcher.stop();
	network_thread.interrupt();
	network_thread.join();
}

unsigned network::proactor::upload_rate()
{
	return Rate_Limit.upload();
}
