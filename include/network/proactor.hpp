#ifndef H_NETWORK_PROACTOR
#define H_NETWORK_PROACTOR

//custom
#include "dispatcher.hpp"
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
		Dispatcher(connect_call_back, disconnect_call_back)
	{}

	/*
	Start async connection to host. Connect call back will happen if connects,
	of disconnect call back if connection fails.
	*/
	void connect(const std::string & host, const std::string & port)
	{
		Thread_Pool.enqueue(boost::bind(&proactor::resolve, this, host, port));
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

	//returns download rate averaged over a few seconds (B/s)
	unsigned download_rate()
	{
		return Rate_Limit.download();
	}

	//returns port listening on (empty if not listening)
	std::string listen_port()
	{
		return Listener.port();
	}

	//get maximum download rate
	unsigned max_download_rate()
	{
		return Rate_Limit.max_download();
	}

	//set maximum download rate
	void max_download_rate(const unsigned rate)
	{
		Rate_Limit.max_download(rate);
	}

	//get maximum upload rate
	unsigned max_upload_rate()
	{
		return Rate_Limit.max_upload();
	}

	//set maximum upload rate
	void max_upload_rate(const unsigned rate)
	{
		Rate_Limit.max_upload(rate);
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

	//start proactor
	bool start()
	{
		network_thread = boost::thread(boost::bind(&proactor::network_loop, this));
		Dispatcher.start();
		return true;
	}

	//start proactor with listening socket, return false if failed to start
	bool start(const endpoint & E)
	{
		assert(E.type() == tcp);
		Listener.open(E);
		if(!Listener.is_open()){
			LOGGER << "failed to start listener";
			return false;
		}
		Listener.set_non_blocking();
		add_socket(std::make_pair(Listener.socket(), boost::shared_ptr<connection>()));
		return start();
	}

	/*
	Stop proactor.
	Note: It's not supported to start the proactor after it has been stopped.
	*/
	void stop()
	{
		Thread_Pool.clear_stop();
		Dispatcher.stop();
		network_thread.interrupt();
		network_thread.join();
	}

	//returns upload rate averaged over a few seconds (B/s)
	unsigned upload_rate()
	{
		return Rate_Limit.upload();
	}

private:

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
	};

	class connection : private boost::noncopyable
	{
	private:
		bool connected;          //false if async connect in progress
		ID_manager & ID_Manager; //allocates and deallocates connection_ID
		std::set<endpoint> E;    //endpoints to try to connect to
		std::time_t last_seen;   //last time there was activity on socket
		int socket_FD;           //file descriptor, will change after open_async()

	public:
		/*
		Ctor for outgoing connection.
		Note: This ctor does DNS resolution if needed.
		*/
		connection(
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
		}

		//ctor for incoming connection
		connection(
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
				N->socket(), "", N->remote_IP(), N->remote_port(), incoming));
		}

		~connection()
		{
			ID_Manager.deallocate(connection_ID);
		}

		//modified by proactor
		bool disc_on_empty; //if true we disconnect when send_buf is empty
		buffer send_buf;    //bytes to send appended to this

		//const
		const boost::shared_ptr<nstream> N; //connection object
		const std::string host;             //hostname or IP to connect to
		const std::string port;             //port number
		const int connection_ID;            //see above

		/* WARNING
		This should not be modified by any thread other than a dispatcher
		thread. Modifying this from the network_thread is not thread safe unless
		it hasn't yet been passed to dispatcher.
		*/
		boost::shared_ptr<connection_info> CI;

		//returns true if async connect in progress
		bool half_open()
		{
			return !connected;
		}

		/*
		Returns true if async connecte succeeded. This is called after half open
		socket becomes writeable.
		Postcondition: If true returned then half_open() will return false.
		*/
		bool is_open()
		{
			if(N->is_open_async()){
				connected = true;
				return true;
			}
			return false;
		}

		/*
		Open async connection, returns false if couldn't allocate socket.
		Postcondition: socket_FD, and CI will change.
		*/
		bool open_async()
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
				connection_ID, N->socket(), host, E.begin()->IP(), port, outgoing));
			E.erase(E.begin());
			return socket_FD != -1;
		}

		/*
		Returns file descriptor.
		Note: Will return different file descriptor after open_async().
		*/
		int socket()
		{
			return socket_FD;
		}

		//returns true if connection timed out
		bool timed_out()
		{
			return std::time(NULL) - last_seen > idle_timeout;
		}

		//resets timer used for timeout
		void touch()
		{
			last_seen = std::time(NULL);
		}
	};

	boost::thread network_thread;
	ID_manager ID_Manager;
	dispatcher Dispatcher;
	listener Listener;
	rate_limit Rate_Limit;
	thread_pool Thread_Pool;
	select Select;
	std::set<int> read_FDS, write_FDS;                    //sets to monitor with select
	std::map<int, boost::shared_ptr<connection> > Socket; //socket_FD associated with connection
	std::map<int, boost::shared_ptr<connection> > ID;     //connection_ID associated with connection

	//function proxy, function calls for network_thread to make
	boost::mutex network_thread_call_mutex;
	std::deque<boost::function<void ()> > network_thread_call;

	/*
	Add socket. P.first must be set to socket_FD.
	Precondition: N and CI must not be empty shared_ptrs.
	*/
	void add_socket(std::pair<int, boost::shared_ptr<connection> > P)
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

	void append_send_buf(const int connection_ID, boost::shared_ptr<buffer> B)
	{
		std::pair<int, boost::shared_ptr<connection> > P = lookup_ID(connection_ID);
		if(P.second){
			P.second->send_buf.append(*B);
			write_FDS.insert(P.second->socket());
		}
	}

	//disconnect sockets which have timed out
	void check_timeouts()
	{
		/*
		We save the elements we want to erase because calling remove_socket
		invalidates iterators for the Socket container.
		*/
		std::vector<std::pair<int, boost::shared_ptr<connection> > > timed_out;
		for(std::map<int, boost::shared_ptr<connection> >::iterator
			iter_cur = Socket.begin(), iter_end = Socket.end(); iter_cur != iter_end;
			++iter_cur)
		{
			if(iter_cur->second->timed_out()){
				timed_out.push_back(*iter_cur);
			}
		}

		for(std::vector<std::pair<int, boost::shared_ptr<connection> > >::iterator
			iter_cur = timed_out.begin(), iter_end = timed_out.end();
			iter_cur != iter_end; ++iter_cur)
		{
			LOGGER << "timed out " << iter_cur->second->N->remote_IP();
			remove_socket(iter_cur->first);
			Dispatcher.disconnect(iter_cur->second->CI);
		}
	}

	//disconnect socket, or schedule disconnect when send_buf empty
	void disconnect(const int connection_ID, const bool on_empty)
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

	//return connection associated with the socket file descriptor
	std::pair<int, boost::shared_ptr<connection> > lookup_socket(const int socket_FD)
	{
		std::map<int, boost::shared_ptr<connection> >::iterator
			iter = Socket.find(socket_FD);
		if(iter == Socket.end()){
			return std::pair<int, boost::shared_ptr<connection> >();
		}else{
			return *iter;
		}
	}

	//return connection associated with connection ID
	std::pair<int, boost::shared_ptr<connection> > lookup_ID(const int connection_ID)
	{
		std::map<int, boost::shared_ptr<connection> >::iterator
			iter = ID.find(connection_ID);
		if(iter == ID.end()){
			return std::pair<int, boost::shared_ptr<connection> >();
		}else{
			return *iter;
		}
	}

	/*
	Called when a async connecting socket becomes writeable. P.first must be
	socket_FD.
	*/
	void handle_async_connection(std::pair<int, boost::shared_ptr<connection> > P)
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
					std::pair<int, boost::shared_ptr<connection> > P(N->socket(),
						boost::shared_ptr<connection>(new connection(ID_Manager, N)));
					add_socket(P);
					Dispatcher.connect(P.second->CI);
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

	void process_network_thread_call()
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

	//remove socket and connection
	void remove_socket(const int socket_FD)
	{
		assert(socket_FD != -1);
		read_FDS.erase(socket_FD);
		write_FDS.erase(socket_FD);
		std::pair<int, boost::shared_ptr<connection> > P = lookup_socket(socket_FD);
		if(P.second){
			Socket.erase(socket_FD);
			ID.erase(P.second->connection_ID);
		}
	}

	/*
	Called by Thread_Pool thread pool thread.
	Resolves host and starts async connect.
	Note: Threads run here. Be careful of data shared with network_thread.
	*/
	void resolve(const std::string & host, const std::string & port)
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
};
}//end namespace network
#endif
