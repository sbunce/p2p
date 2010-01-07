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

/*
-add connect timeout
-add start_listener function
-make rate limiting more granular than 1 second
*/

namespace network{
class proactor : private boost::noncopyable
{
public:
	//if listener not specified then listener is not started
	proactor(
		const boost::function<void (connection_info &)> & connect_call_back,
		const boost::function<void (connection_info &)> & disconnect_call_back,
		const std::string & listen_port = "-1"
	):
		Call_Back_Dispatcher(connect_call_back, disconnect_call_back),
		Resolve_TP(1),
		begin_FD(0),
		end_FD(1)
	{
		FD_ZERO(&read_FDS);
		FD_ZERO(&write_FDS);
		add_socket(std::make_pair(Select_Interrupter.socket(), boost::shared_ptr<state>()));
		if(listen_port != "-1"){
			std::set<endpoint> E = get_endpoint("", listen_port, tcp);
			assert(!E.empty());
			Listener = boost::shared_ptr<listener>(new listener(*E.begin()));
			assert(Listener->is_open());
			Listener->set_non_blocking();
			add_socket(std::make_pair(Listener->socket(), boost::shared_ptr<state>()));
		}
	}

	/*
	Start async connection to host. Connect call back will happen if connects,
	of disconnect call back if connection fails.
	*/
	void connect(const std::string & host, const std::string & port,
		const sock_type type)
	{
		Resolve_TP.queue(boost::bind(&proactor::resolve, this, host, port, type));
	}

	void disconnect(const int connection_ID)
	{
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		network_thread_call.push_back(boost::bind(&proactor::disconnect, this,
			connection_ID, false));
	}

	void disconnect_on_empty(const int connection_ID)
	{
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		network_thread_call.push_back(boost::bind(&proactor::disconnect, this,
			connection_ID, true));
	}

	//returns download rate (averaged over a few seconds)
	unsigned download_rate()
	{
		return Rate_Limit.download();
	}

	//returns maximum allowed download rate
	unsigned max_download_rate()
	{
		return Rate_Limit.max_download();
	}

	//sets maximum allowed download rate
	void max_download_rate(const unsigned rate)
	{
		Rate_Limit.max_download(rate);
	}

	//returns maximum allowed upload rate
	unsigned max_upload_rate()
	{
		return Rate_Limit.max_upload();
	}

	//sets maximum allowed upload rate
	void max_upload_rate(const unsigned rate)
	{
		Rate_Limit.max_upload(rate);
	}

	/*
	Async write data to specified connection.
	Postcondition: send_buf empty.
	*/
	void send(const int connection_ID, buffer & send_buf)
	{
		if(!send_buf.empty()){
			boost::mutex::scoped_lock lock(network_thread_call_mutex);

			//swap buffers to avoid copy
			boost::shared_ptr<buffer> buf(new buffer());
			buf->swap(send_buf);

			network_thread_call.push_back(boost::bind(&proactor::append_send_buf, this,
				connection_ID, buf));
			Select_Interrupter.trigger();
		}
	}

	//start all proactor threads
	boost::mutex start_mutex;
	void start()
	{
		boost::mutex::scoped_lock lock(start_mutex);
		//assert network thread not started
		assert(network_thread.get_id() == boost::thread::id());
		network_thread = boost::thread(boost::bind(&proactor::network_loop, this));
		Call_Back_Dispatcher.start();
	}

	//stop all proactor threads
	boost::mutex stop_mutex;
	void stop()
	{
		boost::mutex::scoped_lock lock(stop_mutex);
		//assert network thread is started
		assert(network_thread.get_id() != boost::thread::id());

		Resolve_TP.clear();           //cancel resolve jobs
		Resolve_TP.interrupt_join();  //interrupt and wait for threads to die

		//stop network_thread
		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		//Note: This is pushed on the front to give it priority.
		network_thread_call.push_front(boost::bind(&proactor::shutdown, this));
		}//END lock scope
		Select_Interrupter.trigger();

		network_thread.join();        //wait for network thread to die
		Call_Back_Dispatcher.stop();  //cancel call backs, stop call back threads
	}

	//returns upload rate (averaged over a few seconds)
	unsigned upload_rate()
	{
		return Rate_Limit.upload();
	}

private:
	boost::thread network_thread;
	call_back_dispatcher Call_Back_Dispatcher;
	select_interrupter Select_Interrupter;
	rate_limit Rate_Limit;

	//used for async DNS resolution, calls proactor::resolve()
	thread_pool Resolve_TP;

	//stuff needed for select()
	fd_set read_FDS;  //set of sockets that need to read
	fd_set write_FDS; //set of sockets that need to write
	int begin_FD;     //first socket
	int end_FD;       //one past last socket

	//listener socket (empty if no listener)
	boost::shared_ptr<listener> Listener;

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
		Note: Once this is instantiated it should not be modified except by the
			call backs.
		*/
		boost::shared_ptr<connection_info> CI;

		//returns true if socket timed out
		bool timed_out()
		{
			return std::time(NULL) - last_seen > 16;
		}

		/*
		Updates the last seen time used when checking timeouts. This is called
		whenever there is activity on the socket.
		*/
		void touch()
		{
			last_seen = std::time(NULL);
		}

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
					P.second->CI->port, P.second->CI->type, outgoing));
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
		std::time_t last_check_timeouts(std::time(NULL));
		fd_set tmp_read_FDS;
		fd_set tmp_write_FDS;
		while(true){
			boost::this_thread::interruption_point();
			if(last_check_timeouts != std::time(NULL)){
				//only check time outs once per second
				check_timeouts();
				last_check_timeouts = std::time(NULL);
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

			//timeout needed because rate limiter won't always let us send/recv data
			timeval tv;
			tv.tv_sec = 0; tv.tv_usec = 1000000 / 100;

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

				if(Listener && FD_ISSET(Listener->socket(), &tmp_read_FDS)){
					//handle incoming connections
					while(boost::shared_ptr<nstream> N = Listener->accept()){
						boost::shared_ptr<connection_info> CI(new connection_info(
							ID_Manager.allocate(), N->socket(), "", Listener->accept_endpoint().IP(),
							Listener->accept_endpoint().port(), tcp, incoming));
						std::pair<int, boost::shared_ptr<state> > P(N->socket(),
							boost::shared_ptr<state>(new state(true, std::set<endpoint>(), N, CI)));
						add_socket(P);
						Call_Back_Dispatcher.connect(P.second->CI);
					}
					FD_CLR(Listener->socket(), &tmp_read_FDS);
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
										Call_Back_Dispatcher.disconnect(P.second->CI);
									}else{
										FD_CLR(P.first, &write_FDS);
										Call_Back_Dispatcher.send(P.second->CI, P.second->send_buf.size());
									}
								}else{
									Call_Back_Dispatcher.send(P.second->CI, P.second->send_buf.size());
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
	void resolve(const std::string & host, const std::string & port,
		const sock_type type)
	{
		std::set<endpoint> E = get_endpoint(host, port, type);
		if(E.empty()){
			//host did not resolve
			boost::shared_ptr<connection_info> CI(new connection_info(ID_Manager.allocate(),
				-1, host, "", port, type, outgoing));
			//deallocate connection_ID right away, it won't be stored outside proactor
			ID_Manager.deallocate(CI->connection_ID);
			Call_Back_Dispatcher.disconnect(CI);
		}else{
			//host resolved, proceed to connection
			boost::shared_ptr<nstream> N(new nstream());
			bool connected = N->open_async(*E.begin());
			boost::shared_ptr<connection_info> CI(new connection_info(ID_Manager.allocate(),
				N->socket(), host, E.begin()->IP(), port, type, outgoing));
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

	//shuts down proactor, disconnect all sockets, clears all state
	void shutdown()
	{
		//disconnect all sockets, except listener
		while(!ID.empty()){
			disconnect(ID.begin()->first, false);
		}

		{//BEGIN lock scope
		boost::mutex::scoped_lock lock(network_thread_call_mutex);
		network_thread_call.clear();
		}//end lock scope

		network_thread.interrupt();
		boost::this_thread::interruption_point();
	}
};
}
#endif
