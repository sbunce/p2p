//THREADSAFE, THREAD-SPAWNING
#ifndef H_REACTOR_SELECT
#define H_REACTOR_SELECT

//include
#include "reactor.hpp"

//standard
#include <deque>

namespace network{
class reactor_select : public network::reactor
{
public:
	reactor_select(
		boost::function<void (socket_data_visible & Socket)> failed_connect_call_back_in,
		boost::function<void (socket_data_visible & Socket)> connect_call_back_in,
		boost::function<void (socket_data_visible & Socket)> disconnect_call_back_in,
		const std::string & port = "-1"
	):
		network::reactor(
			failed_connect_call_back_in,
			connect_call_back_in,
			disconnect_call_back_in
		),
		begin_FD(0),
		end_FD(1),
		listener_IPv4(-1),
		listener_IPv6(-1)
	{
		//initialize fd_set
		FD_ZERO(&master_read_FDS);
		FD_ZERO(&master_write_FDS);
		FD_ZERO(&read_FDS);
		FD_ZERO(&write_FDS);
		FD_ZERO(&connect_FDS);

		//start listeners
		if(port != "-1"){
			wrapper::start_listeners(listener_IPv4, listener_IPv6, port);
			if(listener_IPv4 != -1){
				wrapper::set_non_blocking(listener_IPv4);
				add_socket(listener_IPv4);
			}
			if(listener_IPv6 != -1){
				wrapper::set_non_blocking(listener_IPv6);
				add_socket(listener_IPv6);
			}
		}

		//start self pipe used to get select to return
		wrapper::socket_pair(selfpipe_read, selfpipe_write);
		LOGGER << "selfpipe read " << selfpipe_read << " write " << selfpipe_write;
		wrapper::set_non_blocking(selfpipe_read);
		wrapper::set_non_blocking(selfpipe_write);
		add_socket(selfpipe_read);
	}

	virtual ~reactor_select()
	{
		wrapper::disconnect(listener_IPv4);
		wrapper::disconnect(listener_IPv6);
		wrapper::disconnect(selfpipe_read);
		wrapper::disconnect(selfpipe_write);
	}

	virtual void connect(const std::string & host, const std::string & port,
		boost::shared_ptr<wrapper::info> & Info)
	{
		boost::mutex::scoped_lock lock(connect_pending_mutex);
		if(!Info->resolved()){
			call_back_failed_connect(host, port, FAILED_VALID);
		}else{
			connect_pending.push_back(boost::shared_ptr<connect_job>(new connect_job(host, port, Info)));
			send(selfpipe_write, "0", 1, MSG_NOSIGNAL);
		}
	}

	virtual void finish_call_back(boost::shared_ptr<socket_data> & SD)
	{
		boost::mutex::scoped_lock lock(call_back_finished_mutex);
		call_back_finished.push_back(SD);
		send(selfpipe_write, "0", 1, MSG_NOSIGNAL);
	}

	/*
	Returns the maximum number of connections this reactor supports.
	*/
	static unsigned max_connections_supported()
	{
		#ifdef _WIN32
		/*
		Subtract two for possible listeners (IPv4 and IPv6 listener). Subtract one
		for self-pipe read.
		*/
		return FD_SETSIZE - 3
		#else
		/*
		Subtract two for possible listeners (some POSIX systems don't support
		dual-stack so we subtract two to leave space). Subtract 3 for
		stdin/stdout/stderr. Subtract one for self-pipe read.
		*/
		return FD_SETSIZE - 6;
		#endif
	}

private:
	//thread for main_loop
	boost::thread main_loop_thread;

	fd_set master_read_FDS;  //sockets to check for recv() readyness
	fd_set master_write_FDS; //sockets to check for send() readyness
	fd_set read_FDS;         //used by select()
	fd_set write_FDS;        //used by select()

	/*
	Sockets in progress of connecting. When no bytes can be sent (because of rate
	limit) write_FDS is set equal to this so only connecting sockets are
	monitored (to see when they connect).
	*/
	fd_set connect_FDS;

	int begin_FD; //first socket
	int end_FD;   //one past last socket

	/*
	Listening sockets. If on a dualstack OS then listener_IPv4 = listener_IPv6.
	If -1 then there is no listener.
	*/
	int listener_IPv4;
	int listener_IPv6;

	/*
	This is used for the selfpipe trick. A pipe is opened and put in the fd_set
	select() will monitor. We can asynchronously signal select() to return by
	writing a byte to the read FD of the pipe.
	*/
	int selfpipe_read;  //put in master_FDS
	int selfpipe_write; //written to get select to return
	char selfpipe_discard[512]; //used to discard selfpipe bytes

	/*
	The finish_job function passes off sockets to the select_loop_thread using
	this vector.
	*/
	boost::mutex call_back_finished_mutex;
	std::deque<boost::shared_ptr<socket_data> > call_back_finished;

	/*
	New sockets transferred to select_loop_thread with this. This is needed so
	that the main_loop_thread can set this socket in connect_FDS.
	std::tuple<host, port, info>
	*/
	class connect_job
	{
	public:
		connect_job(
			const std::string & host_in,
			const std::string & port_in,
			boost::shared_ptr<wrapper::info> & Info_in
		):
			host(host_in),
			port(port_in),
			Info(Info_in)
		{}

		std::string host;
		std::string port;
		boost::shared_ptr<wrapper::info> Info;
	};
	boost::mutex connect_pending_mutex;
	std::deque<boost::shared_ptr<connect_job> > connect_pending;

	/*
	These are sockets currently being monitored for activity. Sockets are removed
	from this container and passed to reactor::call_back() when a call back needs
	to be done. When the call back is finished the socket_data is passed to
	reactor_select::finish_call_back() to be added back to this container.
	*/
	std::map<int, boost::shared_ptr<socket_data> > Socket_Data;

	/*
	Add socket to be monitored. This function is used when there is no
	socket_data to be associated with the socket (this is the case with listeners
	and the self-pipe read).
	*/
	void add_socket(const int socket_FD)
	{
		assert(socket_FD >= 0);
		FD_SET(socket_FD, &master_read_FDS);
		if(socket_FD < begin_FD){
			begin_FD = socket_FD;
		}else if(socket_FD >= end_FD){
			end_FD = socket_FD + 1;
		}
	}

	/*
	Add socket to be monitored. This function is used when there is socket_data
	to be associated with the socket.
	*/
	void add_socket(boost::shared_ptr<socket_data> & SD)
	{
		add_socket(SD->socket_FD);
		Socket_Data.insert(std::make_pair(SD->socket_FD, SD));
		if(!SD->send_buff.empty()){
			FD_SET(SD->socket_FD, &master_write_FDS);
		}
	}

	//handle incoming connection on listener
	void establish_incoming(const int listener)
	{
		int new_FD = 0;
		while(new_FD != -1){
			new_FD = wrapper::accept_socket(listener);
			if(new_FD != -1){
				if(incoming_limit_reached()){
					wrapper::disconnect(new_FD);
				}else{
					incoming_increment();
					boost::shared_ptr<socket_data> SD(new socket_data(new_FD, "",
						wrapper::get_IP(new_FD), wrapper::get_port(new_FD)));
					call_back(SD);
				}
			}
		}
	}

	/*
	Retrieves socket_data from Socket_Data container.
	Precondition: Element in Socket_Data with socket_FD as key must exist.
	*/
	boost::shared_ptr<socket_data> get_socket_data(const int socket_FD)
	{
		std::map<int, boost::shared_ptr<socket_data> >::iterator
			iter = Socket_Data.find(socket_FD);
		if(iter == Socket_Data.end()){
			LOGGER << "violated precondition";
			exit(1);
		}
		return iter->second;
	}

	//main network loop
	void main_loop()
	{
		while(true){
			boost::this_thread::interruption_point();

			process_connect();
			process_call_back_finished();
			process_timeouts();

			/*
			Maximum bytes that can be send()'d/recv()'d before exceeding maximum
			upload/download rate. These are decremented by however many bytes are
			send()'d/recv()'d during the socket scan loop.
			*/
			int max_send = Rate_Limit.available_upload();
			int max_recv = Rate_Limit.available_download();

			read_FDS = master_read_FDS;
			if(max_send == 0){
				//rate limit reached, only check for new connections
				write_FDS = connect_FDS;
			}else{
				//check for writeability of all sockets
				write_FDS = master_write_FDS;
			}

			/*
			The select time out is needed because of the rate limiter. If we hit
			the maximum rate we will not send/recv until the current second
			elapses. If select didn't have a timeout it'd get stuck. With the
			timeout select will return and the master_write_FDS will be checked.
			*/
			timeval tv;
			tv.tv_sec = 0; tv.tv_usec = 1000000 / 100;

			int service = select(end_FD, &read_FDS, &write_FDS, NULL, &tv);

			if(service == -1){
				#ifndef _WIN32
				if(errno != EINTR){
					wrapper::error();
				}
				#endif
			}else if(service != 0){
				/*
				There is a starvation problem that can happen when hitting the rate
				limit where the lower sockets hog available bandwidth and the higher
				sockets get starved.

				Because of this we set the maximum send such that in the worst case
				each socket (that needs to be serviced) gets to send/recv an equal
				amount of data.
				*/
				int max_send_per_socket = max_send / service;
				int max_recv_per_socket = max_recv / service;
				if(max_send_per_socket == 0){
					max_send_per_socket = 1;
				}
				if(max_recv_per_socket == 0){
					max_recv_per_socket = 1;
				}

				//socket scan loop
				bool socket_serviced;
				bool need_call_back;
				for(int socket_FD = begin_FD; socket_FD < end_FD && service; ++socket_FD){
					socket_serviced = false;
					need_call_back = false;

					if(socket_FD == selfpipe_read){
						if(FD_ISSET(socket_FD, &read_FDS)){
							recv(socket_FD, selfpipe_discard, sizeof(selfpipe_discard), MSG_NOSIGNAL);
							socket_serviced = true;
						}
						FD_CLR(socket_FD, &read_FDS);
						FD_CLR(socket_FD, &write_FDS);
					}

					if(socket_FD == listener_IPv4){
						if(FD_ISSET(socket_FD, &read_FDS)){
							establish_incoming(socket_FD);
							socket_serviced = true;
						}
						FD_CLR(socket_FD, &read_FDS);
						FD_CLR(socket_FD, &write_FDS);
					}

					if(socket_FD == listener_IPv6){
						if(FD_ISSET(socket_FD, &read_FDS)){
							establish_incoming(socket_FD);
							socket_serviced = true;
						}
						FD_CLR(socket_FD, &read_FDS);
						FD_CLR(socket_FD, &write_FDS);
					}

					if(FD_ISSET(socket_FD, &write_FDS) && FD_ISSET(socket_FD, &connect_FDS)){
						need_call_back = true;
						socket_serviced = true;
						FD_CLR(socket_FD, &write_FDS);
						FD_CLR(socket_FD, &connect_FDS);
						if(!wrapper::async_connect_succeeded(socket_FD)){
							FD_CLR(socket_FD, &read_FDS);
							boost::shared_ptr<socket_data> SD = get_socket_data(socket_FD);
							SD->failed_connect_flag = true;
							SD->error = FAILED_VALID;
						}
					}

					if(FD_ISSET(socket_FD, &read_FDS)){
						main_loop_recv(socket_FD, max_recv_per_socket, max_recv, socket_serviced, need_call_back);
					}

					if(FD_ISSET(socket_FD, &write_FDS)){
						main_loop_send(socket_FD, max_send_per_socket, max_send, socket_serviced, need_call_back);
					}

					if(socket_serviced){
						--service;
					}
					if(need_call_back){
						boost::shared_ptr<socket_data> SD = get_socket_data(socket_FD);
						remove_socket(SD);
						call_back(SD);
					}
				}
			}
		}
	}

	//recv data from a socket
	void main_loop_recv(const int socket_FD, const int max_recv_per_socket,
		int & max_recv, bool & socket_serviced, bool & need_call_back)
	{
		boost::shared_ptr<socket_data> SD = get_socket_data(socket_FD);

		/*
		If the rate limit is such that there is a max_download less
		than the number of sockets to service there is a case where
		not every socket will get to be serviced.
		*/
		int temp;
		if(max_recv_per_socket > max_recv){
			temp = max_recv;
		}else{
			temp = max_recv_per_socket;
		}

		//make sure we don't exceed maximum recv size
		if(temp > 4096){
			temp = 4096;
		}

		if(temp != 0){
			//reserve space for incoming bytes at end of buffer
			SD->recv_buff.tail_reserve(temp);

			/*
			Some implementations might return -1 to indicate an error
			even though select() said the socket was readable. We ignore
			that when that happens.
			*/
			int n_bytes = recv(socket_FD, (char *)SD->recv_buff.tail_start(), temp, MSG_NOSIGNAL);

			if(n_bytes == 0){
				SD->disconnect_flag = true;
				need_call_back = true;
			}else if(n_bytes > 0){
				SD->recv_buff.tail_resize(n_bytes);
				SD->last_seen = std::time(NULL);
				Rate_Limit.add_download_bytes(n_bytes);
				max_recv -= n_bytes;
				SD->recv_flag = true;
				need_call_back = true;
			}
		}
		socket_serviced = true;
	}

	//send data to a socket
	void main_loop_send(const int socket_FD, const int max_send_per_socket,
		int & max_send, bool & socket_serviced, bool & need_call_back)
	{
		boost::shared_ptr<socket_data> SD = get_socket_data(socket_FD);
		if(!SD->send_buff.empty()){
			/*
			If the rate limit is such that there is a max_upload less
			than the number of sockets to service there is a case where
			not every socket will get to be serviced.
			*/
			int temp;
			if(max_send_per_socket > max_send){
				temp = max_send;
			}else{
				temp = max_send_per_socket;
			}

			//don't try to send more than is in the buffer
			if(temp > SD->send_buff.size()){
				temp = SD->send_buff.size();
			}

			//make sure we don't exceed maximum send size
			if(temp > 4096){
				temp = 4096;
			}

			if(temp != 0){
				/*
				Some implementations might return -1 to indicate an error
				even though select() said the socket was writeable. We
				ignore it when that happens.
				*/
				int n_bytes = send(socket_FD, (char *)SD->send_buff.data(),
					temp, MSG_NOSIGNAL);
				if(n_bytes == 0){
					SD->disconnect_flag = true;
					need_call_back = true;
				}else if(n_bytes > 0){
					SD->last_seen = std::time(NULL);
					bool send_buff_empty = SD->send_buff.empty();
					SD->send_buff.erase(0, n_bytes);
					Rate_Limit.add_upload_bytes(n_bytes);
					max_send -= n_bytes;
					if(!send_buff_empty && SD->send_buff.empty()){
						//send_buff went from non-empty to empty
						SD->send_flag = true;
						need_call_back = true;
					}
				}
			}
		}
		socket_serviced = true;
	}

	/*
	Process jobs to start monitoring a socket in progress of connecting. A socket
	will be writeable when it has connected.
	*/
	void process_connect()
	{
		while(true){
			boost::shared_ptr<connect_job> CJ;
			{//begin lock scope
			boost::mutex::scoped_lock lock(connect_pending_mutex);
			if(connect_pending.empty()){
				break;
			}else{
				CJ = connect_pending.front();
				connect_pending.pop_front();
			}
			}//end lock scope

			//check soft connection limit
			if(outgoing_limit_reached()){
				call_back_failed_connect(CJ->host, CJ->port, MAX_CONNECTIONS);
				break;
			}else{
				outgoing_increment();
			}

			int new_FD = wrapper::create_socket(*CJ->Info);
			if(new_FD == -1){
				if(errno == EMFILE){
					//too many open files
					call_back_failed_connect(CJ->host, CJ->port, MAX_CONNECTIONS);
					break;
				}else{
					/*
					Most likely invalid protocol family meaning we will never be able
					to connect to this address.
					*/
					call_back_failed_connect(CJ->host, CJ->port, FAILED_INVALID);
					continue;
				}
			}

			//check hard connection limit
			#ifdef _WIN32
			if(master_read_FDS.fd_count >= FD_SETSIZE){
				call_back_failed_connect(CJ->host, CJ->port, MAX_CONNECTIONS);
				break;
			}
			#else
			if(new_FD >= FD_SETSIZE){
				call_back_failed_connect(CJ->host, CJ->port, MAX_CONNECTIONS);
				break;
			}
			#endif

			//non-blocking required for async connect
			wrapper::set_non_blocking(new_FD);

			if(wrapper::connect_socket(new_FD, *CJ->Info)){
				//socket connected right away, rare but it might happen
				boost::shared_ptr<socket_data> SD(new socket_data(new_FD, CJ->host,
					wrapper::get_IP(*CJ->Info), CJ->port));
				call_back(SD);
			}else{
				//socket in progress of connecting
				boost::shared_ptr<socket_data> SD(new socket_data(new_FD, CJ->host,
					wrapper::get_IP(*CJ->Info), CJ->port, CJ->Info));
				add_socket(SD);
				FD_SET(new_FD, &master_write_FDS);
				FD_SET(new_FD, &connect_FDS);
			}
		}
	}

	/*
	When a worker finishes with a socket it schedules a job to have the network
	thread start monitoring the socket again. This also tells the network thread
	whether it needs to monitor the socket for writeability.
	*/
	void process_call_back_finished()
	{
		while(true){
			boost::shared_ptr<socket_data> SD;
			{//begin lock scope
			boost::mutex::scoped_lock lock(call_back_finished_mutex);
			if(call_back_finished.empty()){
				break;
			}else{
				SD = call_back_finished.front();
				call_back_finished.pop_front();
			}			
			}//end lock scope
			add_socket(SD);
		}
	}

	/*
	Checks to see if any sockets have timed out. If they have schedule a call
	back.
	*/
	void process_timeouts()
	{
		//only check timeouts once per second
		static std::time_t Time = std::time(NULL);
		if(Time == std::time(NULL)){
			return;
		}else{
			Time = std::time(NULL);
		}

		std::map<int, boost::shared_ptr<socket_data> >::iterator
			iter_cur = Socket_Data.begin(), iter_end = Socket_Data.end();
		while(iter_cur != iter_end){
			if(std::time(NULL) - iter_cur->second->last_seen >= 4){
				boost::shared_ptr<socket_data> SD = iter_cur->second;
				Socket_Data.erase(iter_cur++);
				SD->failed_connect_flag = true;
				SD->error = FAILED_VALID;
				remove_socket(SD->socket_FD);
				call_back(SD);
			}else{
				++iter_cur;
			}
		}
	}

	/*
	Stops socket from being monitored. Does not remove the Socket_Data element
	associated with the socket_FD.
	*/
	void remove_socket(const int socket_FD)
	{
		FD_CLR(socket_FD, &master_read_FDS);
		FD_CLR(socket_FD, &master_write_FDS);
		FD_CLR(socket_FD, &read_FDS);
		FD_CLR(socket_FD, &write_FDS);
		FD_CLR(socket_FD, &connect_FDS);

		//update end_FD
		for(int x=end_FD; x>begin_FD; --x){
			if(FD_ISSET(x, &master_read_FDS)){
				end_FD = x + 1;
				break;
			}
		}
		//update begin_FD
		for(int x=begin_FD; x<end_FD; ++x){
			if(FD_ISSET(x, &master_read_FDS)){
				begin_FD = x;
				break;
			}
		}
	}

	/*
	Stops socket from being monitored. Removes the Socket_Data element associated
	with the socket_FD.
	Precondition: socket_data must exist in Socket_Data.
	*/
	void remove_socket(boost::shared_ptr<socket_data> & SD)
	{
		if(Socket_Data.erase(SD->socket_FD) != 1){
			LOGGER << "precondition violated";
			exit(1);
		}
		remove_socket(SD->socket_FD);
	}

	void start_networking()
	{
		main_loop_thread = boost::thread(boost::bind(&reactor_select::main_loop, this));
	}

	void stop_networking()
	{
		main_loop_thread.interrupt();
		main_loop_thread.join();
	}
};
}//end of network namespace
#endif
