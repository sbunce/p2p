/*
This reactor is the most portable. It uses the select() call which is the most
portable way of doing I/O multiplexing. The select() call is synchronous but we
make it somewhat asynchronous by handing off jobs to another thread.
*/
//THREADSAFE, THREAD-SPAWNING
#ifndef H_NETWORK_REACTOR_SELECT
#define H_NETWORK_REACTOR_SELECT

//custom
#include "reactor.hpp"

namespace network{
class reactor_select : public reactor
{
public:
	reactor_select(
		const std::string & port = "-1"
	):
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

	~reactor_select()
	{
		close(listener_IPv4);
		close(listener_IPv6);
		close(selfpipe_read);
		close(selfpipe_write);
	}

	/*
	Returns the maximum number of connections this reactor supports.
	*/
	virtual unsigned max_connections_supported()
	{
		#ifdef _WIN32
		/*
		Subtract two for possible listeners (IPv4 and IPv6 listener). Subtract one
		for self-pipe read.
		*/
		return FD_SETSIZE - 3;
		#else
		/*
		Subtract two for possible listeners (some POSIX systems don't support
		dual-stack so we subtract two to leave space). Subtract 3 for
		stdin/stdout/stderr. Subtract two for self-pipe.
		*/
		return FD_SETSIZE - 7;
		#endif
	}

	virtual void start()
	{
		boost::mutex::scoped_lock lock(start_stop_mutex);
		if(main_loop_thread.get_id() != boost::thread::id()){
			LOGGER << "start called on already started reactor";
			exit(1);
		}
		main_loop_thread = boost::thread(boost::bind(&reactor_select::main_loop, this));
	}

	virtual void stop()
	{
		boost::mutex::scoped_lock lock(start_stop_mutex);
		if(main_loop_thread.get_id() == boost::thread::id()){
			LOGGER << "stop called on already stopped reactor";
			exit(1);
		}
		main_loop_thread.interrupt();
		main_loop_thread.join();
	}

	virtual void trigger_selfpipe()
	{
		send(selfpipe_write, "0", 1, MSG_NOSIGNAL);
	}

private:
	//mutex for start/stop
	boost::mutex start_stop_mutex;

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
	The sock's in this container are monitored by select for activity. When
	something is done to the sock (which sets a flag) the sock is removed from
	this container and put in the job container so a client can deal with the
	sock.
	*/
	std::map<int, boost::shared_ptr<sock> > Monitored;

	/*
	Add socket to be monitored. This function is used when there is no
	sock to be associated with the socket (this is the case with listeners
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
	Add socket to be monitored. This function is used when there is sock
	to be associated with the socket.
	*/
	void add_socket(boost::shared_ptr<sock> & S)
	{
		add_socket(S->socket_FD);
		Monitored.insert(std::make_pair(S->socket_FD, S));
		if(!S->send_buff.empty()){
			FD_SET(S->socket_FD, &master_write_FDS);
		}
	}

	//start async connection process for waiting socks
	void check_connect()
	{
		boost::shared_ptr<sock> S;
		while(S = get_job_connect()){
/*
//check soft connection limit
if(outgoing_limit_reached()){
	call_back_failed_connect(CJ->host, CJ->port, MAX_CONNECTIONS);
	break;
}else{
	outgoing_increment();
}
*/
			//check hard connection limit
			#ifdef _WIN32
			if(master_read_FDS.fd_count >= FD_SETSIZE){
			#else
			if(S->socket_FD >= FD_SETSIZE){
			#endif
				add_job(S);
				return;
			}

			if(wrapper::connect(S->socket_FD, *S->info)){
				//socket connected right away, rare but it might happen
				add_job(S);
			}else{
				//socket in progress of connecting
				add_socket(S);
				FD_SET(S->socket_FD, &master_write_FDS);
				FD_SET(S->socket_FD, &connect_FDS);
			}
		}
	}

	//start monitoring socks the client is done with
	void check_job_finished()
	{
		boost::shared_ptr<sock> S;
		while(S = get_job_finished()){
			add_socket(S);
		}
	}

	//checks timeouts once per second
	void check_timeouts()
	{
/*
Each socket should have it's own timeout that the client can set.
*/

		//only check timeouts once per second
		static std::time_t Time = std::time(NULL);
		if(Time == std::time(NULL)){
			return;
		}else{
			Time = std::time(NULL);
		}
/*
		std::map<int, boost::shared_ptr<sock> >::iterator
			iter_cur = Monitored.begin(), iter_end = Monitored.end();
		while(iter_cur != iter_end){
			if(std::time(NULL) - iter_cur->second->last_seen >= 4){
				boost::shared_ptr<sock> S = iter_cur->second;
				Monitored.erase(iter_cur++);
				S->disconnect_flag = true;
				S->error = TIMED_OUT;
				remove_socket(S->socket_FD);
				call_back(S);
			}else{
				++iter_cur;
			}
		}
*/
	}

	//handle incoming connection on listener
	void establish_incoming(const int listener)
	{
		int new_FD = 0;
		while(new_FD != -1){
			new_FD = wrapper::accept(listener);
			if(new_FD != -1){
//				if(incoming_limit_reached()){
//					close(new_FD);
//				}else{
//incoming_increment();
					boost::shared_ptr<sock> S(new sock(new_FD));
					add_job(S);
//				}
			}
		}
	}

	/*
	Retrieves sock from Monitored container.
	Precondition: Element in Monitored with socket_FD as key must exist.
	*/
	boost::shared_ptr<sock> get_monitored(const int socket_FD)
	{
		std::map<int, boost::shared_ptr<sock> >::iterator
			iter = Monitored.find(socket_FD);
		if(iter == Monitored.end()){
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

			check_connect();
			check_job_finished();
			check_timeouts();

			timeval tv;

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

				/*
				The select() time out is needed because of the rate limiter. If we
				hit the maximum rate we will not send/recv until the current second
				elapses. If select() didn't have a timeout select could get stuck.
				With the timeout select will return and the master_write_FDS will be
				checked.
				*/
				tv.tv_sec = 0; tv.tv_usec = 1000000 / 100;
			}else{
				//check for writeability of all sockets
				write_FDS = master_write_FDS;

				/*
				When the rate limit is not maxed out there is no reason to have a
				short sleep.
				*/
				tv.tv_sec = 1; tv.tv_usec = 0;
			}

			int service = select(end_FD, &read_FDS, &write_FDS, NULL, &tv);

			if(service == -1){
				if(errno != EINTR){
					//profilers can cause this, it's safe to ignore
					LOGGER << errno;
					exit(1);
				}
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
				bool schedule_job;
				for(int socket_FD = begin_FD; socket_FD < end_FD && service; ++socket_FD){
					socket_serviced = false;
					schedule_job = false;

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
						socket_serviced = true;
						FD_CLR(socket_FD, &read_FDS);
						FD_CLR(socket_FD, &write_FDS);
						FD_CLR(socket_FD, &connect_FDS);
						if(wrapper::async_connect_succeeded(socket_FD)){
							schedule_job = true;
						}else{
							//see if there is another address to try
							boost::shared_ptr<sock> S = get_monitored(socket_FD);
							S->info->next();
							if(S->info->resolved()){
								reactor::connect(S);
							}else{
								schedule_job = true;
								S->failed_connect_flag = true;
							}
						}
					}

					if(FD_ISSET(socket_FD, &read_FDS)){
						main_loop_recv(socket_FD, max_recv_per_socket, max_recv, socket_serviced, schedule_job);
					}

					if(FD_ISSET(socket_FD, &write_FDS)){
						main_loop_send(socket_FD, max_send_per_socket, max_send, socket_serviced, schedule_job);
					}

					if(socket_serviced){
						--service;
					}
					if(schedule_job){
						boost::shared_ptr<sock> S = get_monitored(socket_FD);
						remove_socket(S);
						add_job(S);
					}
				}
			}
		}
	}

	//recv data from a socket
	void main_loop_recv(const int socket_FD, const int max_recv_per_socket,
		int & max_recv, bool & socket_serviced, bool & schedule_job)
	{
		boost::shared_ptr<sock> S = get_monitored(socket_FD);
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
			S->recv_buff.tail_reserve(temp);

			/*
			Some implementations might return -1 to indicate an error
			even though select() said the socket was readable. We ignore
			that when that happens.
			*/
			int n_bytes = recv(socket_FD, reinterpret_cast<char *>(S->recv_buff.tail_start()),
				temp, MSG_NOSIGNAL);

			if(n_bytes == 0){
				S->disconnect_flag = true;
				schedule_job = true;
			}else if(n_bytes > 0){
				S->recv_buff.tail_resize(n_bytes);
				S->seen();
				Rate_Limit.add_download_bytes(n_bytes);
				max_recv -= n_bytes;
				S->recv_flag = true;
				schedule_job = true;
			}
		}
		socket_serviced = true;
	}

	//send data to a socket
	void main_loop_send(const int socket_FD, const int max_send_per_socket,
		int & max_send, bool & socket_serviced, bool & schedule_job)
	{
		boost::shared_ptr<sock> S = get_monitored(socket_FD);
		if(!S->send_buff.empty()){
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
			if(temp > S->send_buff.size()){
				temp = S->send_buff.size();
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
				int n_bytes = send(socket_FD, reinterpret_cast<char *>(S->send_buff.data()),
					temp, MSG_NOSIGNAL);
				if(n_bytes == 0){
					S->disconnect_flag = true;
					schedule_job = true;
				}else if(n_bytes > 0){
					S->seen();
					bool send_buff_empty = S->send_buff.empty();
					S->send_buff.erase(0, n_bytes);
					Rate_Limit.add_upload_bytes(n_bytes);
					max_send -= n_bytes;
					if(!send_buff_empty && S->send_buff.empty()){
						//send_buff went from non-empty to empty
						S->send_flag = true;
						schedule_job = true;
					}
				}
			}
		}
		socket_serviced = true;
	}

	/*
	Stops socket from being monitored. Does not remove the Monitored element
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
	Stops socket from being monitored. Removes the Monitored element associated
	with the socket_FD.
	Precondition: sock must exist in Monitored.
	*/
	void remove_socket(boost::shared_ptr<sock> & S)
	{
		if(Monitored.erase(S->socket_FD) != 1){
			LOGGER << "precondition violated";
			exit(1);
		}
		remove_socket(S->socket_FD);
	}
};
}//end of network namespace
#endif
