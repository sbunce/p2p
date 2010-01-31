#ifndef H_NETWORK_PROACTOR
#define H_NETWORK_PROACTOR

//custom
#include "call_back_dispatcher.hpp"
#include "connection_info.hpp"
#include "listener.hpp"
#include "protocol.hpp"
#include "rate_limit.hpp"
#include "select.hpp"

//include
#include <atomic_int.hpp>
#include <boost/shared_ptr.hpp>
#include <thread_pool.hpp>

//standard
#include <map>

namespace network{
class proactor : private boost::noncopyable
{
	/*
	Timeout (seconds) before idle sockets disconnected. This has to be quite high
	otherwise client that download very slowly will be erroneously disconnected.
	*/
	static const unsigned idle_timeout = 600;

	class init
	{
	public:
		init(){ network::start(); }
		~init(){ network::stop(); }
	} Init;

public:
	//if listener not specified then listener is not started
	proactor(
		const boost::function<void (connection_info &)> & connect_call_back,
		const boost::function<void (connection_info &)> & disconnect_call_back
	):
		Call_Back_Dispatcher(connect_call_back, disconnect_call_back),
		Thread_Pool(1)
	{}

	/*
	Start async connection to host. Connect call back will happen if connects,
	of disconnect call back if connection fails.
	*/
	void connect(const std::string & host, const std::string & port)
	{
		Thread_Pool.queue(boost::bind(&proactor::resolve, this, host, port));
	}

	//disconnect as soon as possible
	void disconnect(const int connection_ID)
	{
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		network_thread_call.push_back(boost::bind(&proactor::disconnect, this,
			connection_ID, false));
		Select.interrupt();
	}

	//disconnect when send buffer becomes empty
	void disconnect_on_empty(const int connection_ID)
	{
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		network_thread_call.push_back(boost::bind(&proactor::disconnect, this,
			connection_ID, true));
		Select.interrupt();
	}

	//send data to specified connection
	void send(const int connection_ID, buffer & send_buf)
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

	/* Info / Options
	download_rate:
		Returns download rate (averaged over a few seconds).
	listen_port:
		Returns port we're listening on, or empty string if not listening on any
		port.
	max_download_rate:
		Get or set maximum allowed download rate.
	max_upload_rate:
		Get or set maximum allowed upload rate.
	upload_rate:
		Returns upload rate (averaged over a few seconds).
	*/
	unsigned download_rate(){ return Rate_Limit.download(); }
	std::string listen_port(){ return Listener.port(); }
	unsigned max_download_rate(){ return Rate_Limit.max_download(); }
	void max_download_rate(const unsigned rate){ Rate_Limit.max_download(rate); }
	unsigned max_upload_rate(){ return Rate_Limit.max_upload(); }
	void max_upload_rate(const unsigned rate){ Rate_Limit.max_upload(rate); }
	unsigned upload_rate(){ return Rate_Limit.upload(); }

	//start proactor, return false if fail to start
	bool start()
	{
		network_thread = boost::thread(boost::bind(&proactor::startup, this));
		return true;
	}

	/*
	Start proactor, and optionally start a listener. Returns false if proactor
	failed to start.
	Precondition: proactor must be stopped.
	*/
	bool start(const endpoint & E)
	{
		boost::recursive_mutex::scoped_lock lock(start_stop_mutex);
		if(network_thread.get_id() != boost::thread::id()){
			//proactor already started
			return false;
		}
		assert(E.type() == tcp);
		Listener.open(E);
		if(!Listener.is_open()){
			LOGGER << "failed to start listener";
			return false;
		}
		Listener.set_non_blocking();
		add_socket(std::make_pair(Listener.socket(), boost::shared_ptr<state>()));
		return start();
	}

	/*
	Stop proactor.
	Precondition: proactor must be started.
	*/
	void stop()
	{
		boost::recursive_mutex::scoped_lock lock(start_stop_mutex);
		if(network_thread.get_id() == boost::thread::id()){
			//proactor already stopped
			return;
		}
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		network_thread_call.push_front(boost::bind(&proactor::shutdown, this));
		}//END lock scope
		Select.interrupt();
		network_thread.join();
	}

	//restart proactor, return false if fail to start
	bool restart()
	{
		boost::recursive_mutex::scoped_lock lock(start_stop_mutex);
		stop();
		return start();
	}

	//restart proactor and start a listener, return false if fail to start
	bool restart(const endpoint & E)
	{
		boost::recursive_mutex::scoped_lock lock(start_stop_mutex);
		stop();
		return start(E);
	}

private:
	boost::recursive_mutex start_stop_mutex;
	boost::thread network_thread;

	//all state associated with the socket
	class state : private boost::noncopyable
	{
	public:
		state(
			const bool connected_in,
			const std::set<endpoint> & E_in,
			const boost::shared_ptr<nstream> & N_in,
			const boost::shared_ptr<connection_info> & CI_in
		):
			connected(connected_in),
			E(E_in),
			N(N_in),
			disconnect_on_empty(false),
			CI(CI_in),
			last_seen(std::time(NULL))
		{}

		bool connected;               //false if async connect in progress
		std::set<endpoint> E;         //endpoints to connect to (E.begin() tried first)
		boost::shared_ptr<nstream> N; //the connection
		bool disconnect_on_empty;     //if true, disconnect when send_buf becomes empty
		buffer send_buf;

		/*
		Connection info passed to call backs.
		Note: Once this is instantiated it is not be modified except by the
			Call_Back_Dispatcher when it knows it's safe.
		*/
		boost::shared_ptr<connection_info> CI;

		/*
		timed_out:
			Returns true if connection timed out.
		touch:
			Updates the last time connection was active (used for timeout).
		*/
		bool timed_out(){ return std::time(NULL) - last_seen > idle_timeout; }
		void touch(){ last_seen = std::time(NULL); }

	private:
		std::time_t last_seen;
	};

	/*
	This class allocates connection_IDs. There are a few problems this class
	addresses:
	1. We don't want to let the socket_FDs leave the proactor because the
		proactor operates on sockets asynchronously and may disconnect/reconnect
		while a call back is taking place.
	2. The connection_IDs would have the same problem as #1 if we reused
		connection_IDs. To avoid the problem we continually allocate
		connection_IDs one greater than the last allocated connection_ID.
	3. It is possible that the connection_IDs will wrap around and reuse existing
		connection_IDs. We maintain a set of used connection_IDs so we can prevent
		this.
	*/
	class ID_manager
	{
	public:
		ID_manager():
			highest_allocated(0)
		{}

		//allocate connection_ID (call deallocate when done with it)
		int allocate()
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

		//deallocate connection_ID
		void deallocate(int connection_ID)
		{
			boost::mutex::scoped_lock lock(Mutex);
			if(allocated.erase(connection_ID) != 1){
				LOGGER << "double free of connection_ID";
				exit(1);
			}
		}

	private:
		boost::mutex Mutex;      //locks all access to object
		int highest_allocated;   //last connection_ID allocated
		std::set<int> allocated; //set of all connection_IDs allocated
	} ID_Manager;

	call_back_dispatcher Call_Back_Dispatcher;
	listener Listener;
	rate_limit Rate_Limit;
	thread_pool Thread_Pool;

	//select wrapper and sets of sockets to monitor
	select Select;
	std::set<int> read_FDS, write_FDS;

	//socket_FD associated with socket state
	std::map<int, boost::shared_ptr<state> > Socket;

	//connection_ID associated with socket state
	std::map<int, boost::shared_ptr<state> > ID;

	/*
	When a thread that is not the network_thread needs to modify data that
	is only accessible to the network_thread is pushes a function object on
	the back of this. The network_thread will then make the call.
	Note: network_thread_call_mutex must lock all access to container.
	*/
	boost::mutex network_thread_call_mutex;
	std::deque<boost::function<void ()> > network_thread_call;

	/*
	Add socket. P.first must be set to socket_FD.
	Precondition: N and CI must not be empty shared_ptrs.
	*/
	void add_socket(std::pair<int, boost::shared_ptr<state> > P)
	{
		assert(P.first != -1);
		if(P.second){
			assert(P.second->N);
			assert(P.second->CI);
			if(P.second->connected){
				read_FDS.insert(P.first);
			}else{
				//async connection in progress
				write_FDS.insert(P.first);
			}
			std::pair<std::map<int, boost::shared_ptr<state> >::iterator, bool>
				ret = Socket.insert(P);
			assert(ret.second);
			P.first = P.second->CI->connection_ID;
			ret = ID.insert(P);
			assert(ret.second);
		}else{
			//the listen socket doesn't have state associated with it
			read_FDS.insert(P.first);
		}
	}

	void append_send_buf(const int connection_ID, boost::shared_ptr<buffer> B)
	{
		std::pair<int, boost::shared_ptr<state> > P = lookup_ID(connection_ID);
		if(P.second){
			P.second->send_buf.append(*B);
			write_FDS.insert(P.second->CI->socket_FD);
		}
	}

	//disconnect sockets which have timed out
	void check_timeouts()
	{
		/*
		We save the elements we want to erase because calling remove_socket
		invalidates iterators for the Socket container.
		*/
		std::vector<std::pair<int, boost::shared_ptr<state> > > timed_out;
		for(std::map<int, boost::shared_ptr<state> >::iterator
			iter_cur = Socket.begin(), iter_end = Socket.end(); iter_cur != iter_end;
			++iter_cur)
		{
			if(iter_cur->second->timed_out()){
				timed_out.push_back(*iter_cur);
			}
		}

		for(std::vector<std::pair<int, boost::shared_ptr<state> > >::iterator
			iter_cur = timed_out.begin(), iter_end = timed_out.end();
			iter_cur != iter_end; ++iter_cur)
		{
			LOGGER << "timed out " << iter_cur->second->N->remote_IP();
			remove_socket(iter_cur->first);
			Call_Back_Dispatcher.disconnect(iter_cur->second->CI);
		}
	}

	//disconnect socket, or schedule disconnect when send_buf empty
	void disconnect(const int connection_ID, const bool on_empty)
	{
		std::pair<int, boost::shared_ptr<state> > P = lookup_ID(connection_ID);
		if(P.second){
			if(on_empty){
				//disconnect when send_buf empty
				if(P.second->send_buf.empty()){
					remove_socket(P.second->CI->socket_FD);
					Call_Back_Dispatcher.disconnect(P.second->CI);
				}else{
					P.second->disconnect_on_empty = true;
				}
			}else{
				//disconnect regardless of whether send_buf empty
				remove_socket(P.second->CI->socket_FD);
				Call_Back_Dispatcher.disconnect(P.second->CI);
			}
		}
	}

	//return state associated with the socket file descriptor
	std::pair<int, boost::shared_ptr<state> > lookup_socket(const int socket_FD)
	{
		std::map<int, boost::shared_ptr<state> >::iterator
			iter = Socket.find(socket_FD);
		if(iter == Socket.end()){
			return std::pair<int, boost::shared_ptr<state> >();
		}else{
			return *iter;
		}
	}

	//return state associated with connection ID
	std::pair<int, boost::shared_ptr<state> > lookup_ID(const int connection_ID)
	{
		std::map<int, boost::shared_ptr<state> >::iterator
			iter = ID.find(connection_ID);
		if(iter == ID.end()){
			return std::pair<int, boost::shared_ptr<state> >();
		}else{
			return *iter;
		}
	}

	/*
	Called when a async connecting socket becomes writeable. P.first must be
	socket_FD.
	*/
	void handle_async_connection(std::pair<int, boost::shared_ptr<state> > P)
	{
		assert(P.first != -1);
		P.second->touch();
		if(P.second->N->is_open_async()){
			read_FDS.insert(P.first); //monitor socket for incoming data
			write_FDS.erase(P.first); //send_buf will be empty after connect
			P.second->connected = true;
			P.second->E.clear();
			Call_Back_Dispatcher.connect(P.second->CI);
		}else{
			P.second->E.erase(P.second->E.begin());
			remove_socket(P.first);
			if(P.second->E.empty()){
				//no more endpoints to try
				remove_socket(P.first);
				Call_Back_Dispatcher.disconnect(P.second->CI);
			}else{
				//try next endpoint
				P.second->N->open_async(*P.second->E.begin());
				P.first = P.second->N->socket();
				//recreate connection_info with new IP
				P.second->CI = boost::shared_ptr<connection_info>(new connection_info(
					ID_Manager.allocate(), P.first, P.second->CI->host, P.second->E.begin()->IP(),
					P.second->CI->port, outgoing));
				if(P.first == -1){
					//couldn't allocate socket
					remove_socket(P.first);
					Call_Back_Dispatcher.disconnect(P.second->CI);
				}else{
					add_socket(P);
				}
			}
		}
	}

	//networking threads works in this function
	void network_loop()
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
					boost::shared_ptr<connection_info> CI(new connection_info(
						ID_Manager.allocate(), N->socket(), "", N->remote_IP(),
						N->remote_port(), incoming));
					std::pair<int, boost::shared_ptr<state> > P(N->socket(),
						boost::shared_ptr<state>(new state(true, std::set<endpoint>(), N, CI)));
					add_socket(P);
					Call_Back_Dispatcher.connect(P.second->CI);
				}
				tmp_read_FDS.erase(Listener.socket());
			}

			//handle all writes
			while(!tmp_write_FDS.empty()){
				std::pair<int, boost::shared_ptr<state> > P = lookup_socket(*tmp_write_FDS.begin());
				P.second->touch();
				if(!P.second->connected){
					handle_async_connection(P);
				}else{
					//n_bytes initially set to max send, then set to how much sent
					int n_bytes = Rate_Limit.available_upload(tmp_write_FDS.size());
					if(n_bytes != 0){
						n_bytes = P.second->N->send(P.second->send_buf, n_bytes);
						if(n_bytes == -1 || n_bytes == 0){
							remove_socket(P.first);
							Call_Back_Dispatcher.disconnect(P.second->CI);
						}else{
							Rate_Limit.add_upload(n_bytes);
							if(P.second->send_buf.empty()){
								if(P.second->disconnect_on_empty){
									remove_socket(P.first);
									Call_Back_Dispatcher.send(P.second->CI, n_bytes, P.second->send_buf.size());
									Call_Back_Dispatcher.disconnect(P.second->CI);
								}else{
									write_FDS.erase(P.first);
									Call_Back_Dispatcher.send(P.second->CI, n_bytes, P.second->send_buf.size());
								}
							}else{
								Call_Back_Dispatcher.send(P.second->CI, n_bytes, P.second->send_buf.size());
							}
						}
					}
				}
				tmp_write_FDS.erase(tmp_write_FDS.begin());
			}

			//handle all reads
			while(!tmp_read_FDS.empty()){
				std::pair<int, boost::shared_ptr<state> > P = lookup_socket(*tmp_read_FDS.begin());
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
						Call_Back_Dispatcher.disconnect(P.second->CI);
					}else{
						Rate_Limit.add_download(n_bytes);
						Call_Back_Dispatcher.recv(P.second->CI, recv_buf);
					}
				}
				tmp_read_FDS.erase(tmp_read_FDS.begin());
			}
		}
	}

	void process_network_thread_call()
	{
		while(true){
			boost::function<void ()> F;
			{//begin lock scope
			boost::mutex::scoped_lock lock(network_thread_call_mutex);
			if(network_thread_call.empty()){
				break;
			}
			F = network_thread_call.front();
			network_thread_call.pop_front();
			}//end lock scope
			F();
		}
	}

	//remove socket and state
	void remove_socket(const int socket_FD)
	{
		assert(socket_FD != -1);
		read_FDS.erase(socket_FD);
		write_FDS.erase(socket_FD);
		std::pair<int, boost::shared_ptr<state> > P = lookup_socket(socket_FD);
		if(P.second){
			Socket.erase(socket_FD);
			ID_Manager.deallocate(P.second->CI->connection_ID);
			ID.erase(P.second->CI->connection_ID);
		}
	}

	/*
	Called by Thread_Pool thread pool thread.
	Resolves host and starts async connect.
	Note: This function must not access any data member except
		network_thread_call (after locking network_thread_call_mutex).
	*/
	void resolve(const std::string & host, const std::string & port)
	{
		std::set<endpoint> E = get_endpoint(host, port, tcp);
		if(E.empty()){
			//host did not resolve
			boost::shared_ptr<connection_info> CI(new connection_info(ID_Manager.allocate(),
				-1, host, "", port, outgoing));
			//deallocate connection_ID right away, it won't be stored outside proactor
			ID_Manager.deallocate(CI->connection_ID);
			Call_Back_Dispatcher.disconnect(CI);
		}else{
			//host resolved, proceed to connection
			boost::shared_ptr<nstream> N(new nstream());
			bool connected = N->open_async(*E.begin());
			boost::shared_ptr<connection_info> CI(new connection_info(ID_Manager.allocate(),
				N->socket(), host, E.begin()->IP(), port, outgoing));
			std::pair<int, boost::shared_ptr<state> > P(N->socket(),
				boost::shared_ptr<state>(new state(connected, E, N, CI)));
			if(connected){
				//socket connected immediately, rare but can happen
				boost::mutex::scoped_lock lock(network_thread_call_mutex);
				network_thread_call.push_back(boost::bind(&proactor::handle_async_connection, this, P));
			}else if(P.first != -1){
				//in progress of connecting
				boost::mutex::scoped_lock lock(network_thread_call_mutex);
				network_thread_call.push_back(boost::bind(&proactor::add_socket, this, P));
			}else{
				Call_Back_Dispatcher.disconnect(P.second->CI);
			}
		}
		Select.interrupt();
	}

	/*
	Shutdown proactor.
	Disconnects all sockets and does cleanup.
	*/
	void shutdown()
	{
		//disconnect all sockets, except listener
		while(!ID.empty()){
			disconnect(ID.begin()->first, false);
		}

		//disconnect listener
		remove_socket(Listener.socket());
		Listener.close();

		//stop any pending resolve jobs
		Thread_Pool.clear();

		//erase any pending network_thread_call's
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		network_thread_call.clear();
		}//end lock scope

		//stop call back threads
		Call_Back_Dispatcher.stop();

		//terminate this thread
		network_thread.interrupt();
		boost::this_thread::interruption_point();
	}

	/*
	The network_thread starts in this function.
	Do setup and start listener.
	*/
	void startup()
	{
		//start call back threads
		Call_Back_Dispatcher.start();

		//send/recv
		network_loop();
	}
};
}//end namespace network
#endif
