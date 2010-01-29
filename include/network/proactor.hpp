#ifndef H_NETWORK_PROACTOR
#define H_NETWORK_PROACTOR

//custom
#include "call_back_dispatcher.hpp"
#include "connection_info.hpp"
#include "listener.hpp"
#include "protocol.hpp"
#include "rate_limit.hpp"
#include "select_interrupter.hpp"

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

	/*
	Maximum number of times per second select will timeout. This is needed
	because the rate limiter won't always allow us to send bytes.
	*/
	static const unsigned select_timeouts_per_second = 100;

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
		Resolve_TP(1),
		begin_FD(0),
		end_FD(1)
	{
		FD_ZERO(&read_FDS);
		FD_ZERO(&write_FDS);
		add_socket(std::make_pair(Select_Interrupter.socket(), boost::shared_ptr<state>()));
	}

	/*
	Start async connection to host. Connect call back will happen if connects,
	of disconnect call back if connection fails.
	*/
	void connect(const std::string & host, const std::string & port)
	{
		Resolve_TP.queue(boost::bind(&proactor::resolve, this, host, port));
	}

	//disconnect as soon as possible
	void disconnect(const int connection_ID)
	{
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		network_thread_call.push_back(boost::bind(&proactor::disconnect, this,
			connection_ID, false));
		Select_Interrupter.trigger();
	}

	//disconnect when send buffer becomes empty
	void disconnect_on_empty(const int connection_ID)
	{
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		network_thread_call.push_back(boost::bind(&proactor::disconnect, this,
			connection_ID, true));
		Select_Interrupter.trigger();
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
			Select_Interrupter.trigger();
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
		Select_Interrupter.trigger();
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
	//lock for restart(), start(), and stop() functions
	boost::recursive_mutex start_stop_mutex;

	//thread which runs in network_loop(), does all send/recv
	boost::thread network_thread;

	call_back_dispatcher Call_Back_Dispatcher;
	listener Listener;
	rate_limit Rate_Limit;
	select_interrupter Select_Interrupter;

	/*
	Thread pool for DNS resolution.
	Note: There is async DNS resolution on some platforms but not all.
	*/
	thread_pool Resolve_TP;

	//stuff needed for select()
	fd_set read_FDS;  //set of sockets that need to read
	fd_set write_FDS; //set of sockets that need to write
	int begin_FD;     //first socket
	int end_FD;       //one past last socket

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

	//socket_FD associated with socket state
	std::map<int, boost::shared_ptr<state> > Socket;

	//connection_ID associated with socket state
	std::map<int, boost::shared_ptr<state> > ID; 

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
		if(P.first < begin_FD){
			begin_FD = P.first;
		}else if(P.first >= end_FD){
			end_FD = P.first + 1;
		}
		if(P.second){
			assert(P.second->N);
			assert(P.second->CI);
			if(P.second->connected){
				FD_SET(P.first, &read_FDS);
			}else{
				//async connection in progress
				FD_SET(P.first, &write_FDS);
			}
			std::pair<std::map<int, boost::shared_ptr<state> >::iterator, bool>
				ret = Socket.insert(P);
			assert(ret.second);
			P.first = P.second->CI->connection_ID;
			ret = ID.insert(P);
			assert(ret.second);
		}else{
			/*
			The select_interrupter and listener sockets don't have state associated
			with them. They are also read only.
			*/
			FD_SET(P.first, &read_FDS);
		}
	}

	void append_send_buf(const int connection_ID, boost::shared_ptr<buffer> B)
	{
		std::pair<int, boost::shared_ptr<state> > P = lookup_ID(connection_ID);
		if(P.second){
			P.second->send_buf.append(*B);
			FD_SET(P.second->CI->socket_FD, &write_FDS);
		}
	}

	//disconnect sockets which have timed out
	void check_timeouts()
	{
		for(int socket_FD = begin_FD; socket_FD < end_FD; ++socket_FD){
			if(FD_ISSET(socket_FD, &read_FDS) || FD_ISSET(socket_FD, &write_FDS)){
				std::pair<int, boost::shared_ptr<state> > P = lookup_socket(socket_FD);
				if(P.second && P.second->timed_out()){
					LOGGER << "timed out " << P.second->N->remote_IP();
					remove_socket(socket_FD);
					Call_Back_Dispatcher.disconnect(P.second->CI);
				}
			}
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
			FD_SET(P.first, &read_FDS);  //monitor socket for incoming data
			FD_CLR(P.first, &write_FDS); //send_buf will be empty after connect
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
		fd_set tmp_read_FDS;
		fd_set tmp_write_FDS;
		timeval tv;
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
				FD_ZERO(&tmp_read_FDS);
			}else{
				tmp_read_FDS = read_FDS;
			}
			if(Rate_Limit.available_upload() == 0){
				FD_ZERO(&tmp_write_FDS);
			}else{
				tmp_write_FDS = write_FDS;
			}

			/*
			Windows gives a 10022 error (invalid argument) if passed empty fd_sets.
			Don't call select if fd_sets are empty.
			*/
			#ifdef _WIN32
			if(tmp_read_FDS.fd_count == 0 && tmp_write_FDS.fd_count == 0){
				boost::this_thread::sleep(boost::posix_time::milliseconds(
					1000 / select_timeouts_per_second));
				continue;
			}
			#endif

			/*
			On some systems timeval is modified after select returns and set to the
			time that select blocked for. For example, Linux does this but Windows
			does not.
			*/
			tv.tv_sec = 0;
			tv.tv_usec = 1000000 / select_timeouts_per_second;

			int service = select(end_FD, &tmp_read_FDS, &tmp_write_FDS, NULL, &tv);
			if(service == -1){
				//ignore interrupt signal, profilers can cause this
				if(errno != EINTR){
					LOGGER << errno; exit(1);
				}
			}else if(service != 0){
				if(FD_ISSET(Select_Interrupter.socket(), &tmp_read_FDS)){
					Select_Interrupter.reset();
					FD_CLR(Select_Interrupter.socket(), &tmp_read_FDS);
					--service;
				}

				if(Listener.is_open() && FD_ISSET(Listener.socket(), &tmp_read_FDS)){
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
					FD_CLR(Listener.socket(), &tmp_read_FDS);
					--service;
				}

				//collect sockets that need read/write so we can divide bandwidth between them
				std::deque<int> need_read, need_write;
				for(int socket_FD = begin_FD; socket_FD < end_FD && service; ++socket_FD){
					if(FD_ISSET(socket_FD, &tmp_read_FDS)){
						need_read.push_back(socket_FD);
					}
					if(FD_ISSET(socket_FD, &tmp_write_FDS)){
						need_write.push_back(socket_FD);
					}
					if(FD_ISSET(socket_FD, &tmp_read_FDS) || FD_ISSET(socket_FD, &tmp_write_FDS)){
						--service;
					}
				}

				//handle all writes
				while(!need_write.empty()){
					std::pair<int, boost::shared_ptr<state> > P = lookup_socket(need_write.front());
					P.second->touch();
					if(!P.second->connected){
						handle_async_connection(P);
					}else{
						//n_bytes initially set to max send, then set to how much sent
						int n_bytes = Rate_Limit.available_upload(need_write.size());
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
										FD_CLR(P.first, &write_FDS);
										Call_Back_Dispatcher.send(P.second->CI, n_bytes, P.second->send_buf.size());
									}
								}else{
									Call_Back_Dispatcher.send(P.second->CI, n_bytes, P.second->send_buf.size());
								}
							}
						}
					}
					need_write.pop_front();
				}

				//handle all reads
				while(!need_read.empty()){
					std::pair<int, boost::shared_ptr<state> > P = lookup_socket(need_read.front());
					if(P.second){
						P.second->touch();
						//n_bytes initially set to max read, then set to how much read
						int n_bytes = Rate_Limit.available_download(need_read.size());
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
					need_read.pop_front();
				}
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
		FD_CLR(socket_FD, &read_FDS);
		FD_CLR(socket_FD, &write_FDS);
		//update begin_FD
		for(int x=begin_FD; x<end_FD; ++x){
			if(FD_ISSET(x, &read_FDS) || FD_ISSET(x, &write_FDS)){
				begin_FD = x;
				break;
			}
		}
		//update end_FD
		for(int x=end_FD; x>begin_FD; --x){
			if(FD_ISSET(x, &read_FDS) || FD_ISSET(x, &write_FDS)){
				end_FD = x + 1;
				break;
			}
		}
		std::pair<int, boost::shared_ptr<state> > P = lookup_socket(socket_FD);
		if(P.second){
			Socket.erase(socket_FD);
			ID_Manager.deallocate(P.second->CI->connection_ID);
			ID.erase(P.second->CI->connection_ID);
		}
	}

	/*
	Called by Resolve_TP thread pool thread.
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
		Select_Interrupter.trigger();
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
		Resolve_TP.clear();

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
