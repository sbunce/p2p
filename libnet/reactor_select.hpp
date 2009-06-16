//THREADSAFE, THREAD-SPAWNING
#ifndef H_REACTOR_SELECT
#define H_REACTOR_SELECT

//include
#include <reactor.hpp>

//std
#include <deque>

namespace network{
class reactor_select : public network::reactor
{
public:
	reactor_select(
		boost::function<void (const std::string & host, const std::string & port)> failed_connect_call_back_in,
		boost::function<void (socket_data::socket_data_visible & Socket)> connect_call_back_in,
		boost::function<void (socket_data::socket_data_visible & Socket)> disconnect_call_back_in,
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
		max_connections = FD_SETSIZE;

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
		if(!Info->resolved()){
			call_back_failed_connect(host, port);
			return;
		}

		{//begin lock scope
		boost::mutex::scoped_lock lock(connect_pending_mutex);
		connect_pending.push_back(connect_job(host, port, Info));
		send(selfpipe_write, "0", 1, MSG_NOSIGNAL);
		}//end lock scope
	}

	virtual void finish_job(boost::shared_ptr<socket_data> & SD)
	{
		if(SD->disconnect_flag || SD->failed_connect_flag){
			//call back requested socket disconnect
			boost::mutex::scoped_lock lock(disconnect_pending_mutex);
			disconnect_pending.push_back(SD->socket_FD);
		}else{
			//call back finished normally
			boost::mutex::scoped_lock lock(job_finished_mutex);
			job_finished.push_back(SD);
			send(selfpipe_write, "0", 1, MSG_NOSIGNAL);
		}
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
	char selfpipe_discard[1024]; //used to discard selfpipe bytes

	/*
	The finish_job function passes off sockets to the select_loop_thread using
	this vector.
	*/
	boost::mutex job_finished_mutex;
	std::vector<boost::shared_ptr<socket_data> > job_finished;

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
	std::vector<connect_job> connect_pending;

	/*
	Sockets finished_job() needs disconnected. This is done instead of calling
	close() in finish_job() because remove_socket needs to be called so that
	being_FD and end_FD can be called.
	*/
	boost::mutex disconnect_pending_mutex;
	std::vector<int> disconnect_pending;

	/*
	Add socket to master_read_FDS and adjusts begin_FD and end_FD.
	*/
	void add_socket(const int socket_FD)
	{
		assert(socket_FD > 0);
		FD_SET(socket_FD, &master_read_FDS);
		if(socket_FD < begin_FD){
			begin_FD = socket_FD;
		}else if(socket_FD >= end_FD){
			end_FD = socket_FD + 1;
		}
	}

	//handle incoming connection on listener
	void establish_incoming(const int listener)
	{
		int new_FD = 0;
		while(new_FD != -1){
			new_FD = wrapper::accept_socket(listener);
			if(new_FD != -1){
				boost::shared_ptr<socket_data> SD(new socket_data(new_FD, "",
					wrapper::get_IP(new_FD), wrapper::get_port(new_FD)));
				add_socket_data(SD);
				call_back(new_FD);
			}
		}
	}

	//main network loop
	void main_loop()
	{
		std::time_t Time = std::time(NULL);
		while(true){
			boost::this_thread::interruption_point();
			if(Time != std::time(NULL)){
				Time = std::time(NULL);
				process_timeouts();
			}

			process_disconnect();
			process_connect();
			process_job_finished();

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
			tv.tv_sec = 0; tv.tv_usec = 1000000 / 10; //1/10 second

			int service = select(end_FD, &read_FDS, &write_FDS, NULL, &tv);

			if(service == -1){
				#ifndef WIN32
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
							boost::shared_ptr<socket_data> SD = get_socket_data(socket_FD);
							SD->failed_connect_flag = true;
							SD->Error = socket_data::FAILED;
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
						remove_socket(socket_FD);
						call_back(socket_FD);
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
	Process jobs to disconnect a socket.
	*/
	void process_disconnect()
	{
		boost::mutex::scoped_lock lock(disconnect_pending_mutex);
		for(int x=0; x<disconnect_pending.size(); ++x){
			remove_socket(disconnect_pending[x]);
			wrapper::disconnect(disconnect_pending[x]);
			remove_socket_data(disconnect_pending[x]);
		}
		disconnect_pending.clear();
	}

	/*
	Process jobs to start monitoring a socket in progress of connecting. A socket
	will be writeable when it has connected.
	*/
	void process_connect()
	{
		std::vector<connect_job> connect_pending_temp;

		//copy connect_pending so we can unlock ASAP
		{//begin lock scope
		boost::mutex::scoped_lock lock(connect_pending_mutex);
		connect_pending_temp.assign(connect_pending.begin(), connect_pending.end());
		connect_pending.clear();
		}//end lock scope

		std::vector<connect_job>::iterator
			iter_cur = connect_pending_temp.begin(),
			iter_end = connect_pending_temp.end();
		while(iter_cur != iter_end){
			int new_FD = wrapper::create_socket(*iter_cur->Info);
			if(new_FD == -1){
				call_back_failed_connect(iter_cur->host, iter_cur->port);
				++iter_cur;
				continue;
			}

			//non-blocking required for async connect
			wrapper::set_non_blocking(new_FD);

			if(wrapper::connect_socket(new_FD, *iter_cur->Info)){
				//socket connected right away, rare but it might happen
				boost::shared_ptr<socket_data> SD(new socket_data(new_FD,
					iter_cur->host, wrapper::get_IP(*iter_cur->Info), iter_cur->port));
				add_socket_data(SD);
				call_back(new_FD);
			}else{
				//socket in progress of connecting
				add_socket(new_FD);
				boost::shared_ptr<socket_data> SD(new socket_data(new_FD, iter_cur->host,
					wrapper::get_IP(*iter_cur->Info), iter_cur->port, iter_cur->Info));
				add_socket_data(SD);
				FD_SET(new_FD, &master_write_FDS);
				FD_SET(new_FD, &connect_FDS);
			}
			++iter_cur;
		}
	}

	/*
	When a worker finishes with a socket it schedules a job to have the network
	thread start monitoring the socket again. This also tells the network thread
	whether it needs to monitor the socket for writeability.
	*/
	void process_job_finished()
	{
		boost::mutex::scoped_lock lock(job_finished_mutex);
		for(int x=0; x<job_finished.size(); ++x){
			add_socket(job_finished[x]->socket_FD);
			if(!job_finished[x]->send_buff.empty()){
				//send buff contains data, check socket for writeability
				FD_SET(job_finished[x]->socket_FD, &master_write_FDS);
			}
		}
		job_finished.clear();
	}

	/*
	Checks to see if any sockets have timed out. If they have schedule a call
	back.
	*/
	void process_timeouts()
	{
		std::vector<int> timed_out;
		check_timeouts(timed_out);
		for(int x=0; x<timed_out.size(); ++x){
			//if socket failed connection see if there are any more addresses to try
			boost::shared_ptr<socket_data> SD = get_socket_data(timed_out[x]);
			if(FD_ISSET(timed_out[x], &connect_FDS)){
				remove_socket(timed_out[x]);
				SD->Info->next_res();
				if(SD->Info->resolved()){
					//try next address the host resolved to
					connect(SD->host, SD->port, SD->Info);
				}else{
					SD->failed_connect_flag = true;
					SD->Error = socket_data::FAILED;
					call_back(timed_out[x]);
				}
			}else{
				remove_socket(timed_out[x]);
				SD->disconnect_flag = true;
				call_back(timed_out[x]);
			}
		}
	}

	//remove socket from all fd_set's, adjust min_FD and end_FD.
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
