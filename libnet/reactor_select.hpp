//THREADSAFE, THREAD-SPAWNING
#ifndef H_REACTOR_SELECT
#define H_REACTOR_SELECT

//include
#include <rate_limit.hpp>
#include <reactor.hpp>

//std
#include <deque>

class reactor_select : public reactor
{
public:
	reactor_select(
		const std::string port = "-1"
	):
		begin_FD(0),
		end_FD(1),
		listener(-1)
	{
		//initialize fd_set
		FD_ZERO(&master_read_FDS);
		FD_ZERO(&master_write_FDS);
		FD_ZERO(&read_FDS);
		FD_ZERO(&write_FDS);

		if(port != "-1"){
			start_listener(port);
		}

		//start self pipe used to get select to return
		start_selfpipe();

		select_loop_thread = boost::thread(boost::bind(&reactor_select::select_loop, this));
	}

	virtual ~reactor_select()
	{
		select_loop_thread.interrupt();
		select_loop_thread.join();
	}

	virtual bool connect_to(const std::string & host, const std::string & port)
	{
		network_wrapper::info Info = network_wrapper::get_info(host.c_str(), port.c_str());
		if(!Info.resolved()){
			LOGGER << "failed to resolve host " << host << " port " << port;
			return false;
		}

		int new_FD = network_wrapper::create_socket(Info);

		//non-blocking required for async connect
		network_wrapper::set_non_blocking(new_FD);

		if(network_wrapper::connect_socket(new_FD, Info)){
			//socket in progress of connecting
			boost::mutex::scoped_lock lock(connecting_mutex);
			connecting.push_back(boost::shared_ptr<socket_data>(new socket_data(new_FD, host, port)));
		}else{
			//socket connected right away, rare but it might happen
			boost::mutex::scoped_lock lock(job_finished_mutex);
			job_finished.push_back(boost::shared_ptr<socket_data>(new socket_data(new_FD, host, port)));
		}
		send(selfpipe_write, "0", 1, MSG_NOSIGNAL);
		return true;
	}

	virtual void get_job(boost::shared_ptr<socket_data> & info)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		while(job.empty()){
			job_cond.wait(job_mutex);
		}
		info = job.front();
		job.pop_front();
	}

	virtual void finish_job(boost::shared_ptr<socket_data> & info)
	{
		boost::mutex::scoped_lock lock(job_finished_mutex);
		if(info->disconnect_flag){
			boost::mutex::scoped_lock lock(disconnect_mutex);
			disconnect.push_back(info->socket_FD);
		}else if(!info->failed_connect_flag){
			job_finished.push_back(info);
			send(selfpipe_write, "0", 1, MSG_NOSIGNAL);
		}
	}

private:
	//thread for main_loop
	boost::thread select_loop_thread;

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
	int listener; //socket of listener (-1 means no listener)

	/*
	This is used for the selfpipe trick. A pipe is opened and put in the fd_set
	select() will monitor. We can asynchronously signal select() to return by
	writing a byte to the read FD of the pipe.
	*/
	int selfpipe_read;  //put in master_FDS
	int selfpipe_write; //written to get select to return
	char selfpipe_discard[1024]; //used to discard selfpipe bytes

	/*
	When a socket needs to have a call back done it is queued in this container
	along with the socket_data.

	The socket_data doesn't need to be locked because it's guaranteed that the
	select_loop_thread won't touch it.
	*/
	std::deque<boost::shared_ptr<socket_data> > job;
	boost::mutex job_mutex;
	boost::condition_variable_any job_cond;

	/*
	The finish_job function passes off sockets to the select_loop_thread using
	this vector.
	*/
	boost::mutex job_finished_mutex;
	std::vector<boost::shared_ptr<socket_data> > job_finished;

	/*
	New sockets transferred to select_loop_thread with this. If connecting is
	true then the socket is set in connect_FDS.
	std::pair<socket_FD, connecting>
	*/
	boost::mutex connecting_mutex;
	std::vector<boost::shared_ptr<socket_data> > connecting;

	/*
	Sockets finished_job() needs disconnected. This is done instead of calling
	close() in finish_job() because remove_socket needs to be called so that
	being_FD and end_FD can be called.
	*/
	boost::mutex disconnect_mutex;
	std::vector<int> disconnect;

	rate_limit Rate_Limit;

	/*
	Add socket to master_FDS, adjusts begin_FD and end_FD, and create socket
	data unless explictly told not to.
	Note: the selfpipe and listener will set create_socket_data = false;
	*/
	void add_socket(const int socket_FD, bool create_socket_data = true)
	{
		assert(socket_FD > 0);
		FD_SET(socket_FD, &master_read_FDS);

		if(socket_FD < begin_FD){
			begin_FD = socket_FD;
		}else if(socket_FD >= end_FD){
			end_FD = socket_FD + 1;
		}

		if(create_socket_data){
			std::map<int, boost::shared_ptr<socket_data> >::iterator iter = Socket_Data.find(socket_FD);
			assert(iter == Socket_Data.end());
			Socket_Data.insert(std::make_pair(socket_FD, new socket_data(socket_FD,
				network_wrapper::get_IP(socket_FD), network_wrapper::get_port(socket_FD))));
		}
	}

	void check_timeouts()
	{
		//temp vector needed because queue_job invalidates iterator
		std::vector<int> disconnect;
		std::map<int, boost::shared_ptr<socket_data> >::iterator
		iter_cur = Socket_Data.begin(),
		iter_end = Socket_Data.end();
		while(iter_cur != iter_end){
			if(std::time(NULL) - iter_cur->second->last_seen > 8){
				if(FD_ISSET(iter_cur->first, &connect_FDS)){
					iter_cur->second->failed_connect_flag = true;
				}else{
					iter_cur->second->disconnect_flag = true;
				}
				disconnect.push_back(iter_cur->first);
			}
			++iter_cur;
		}

		for(int x=0; x<disconnect.size(); ++x){
			queue_job(disconnect[x]);
		}
		disconnect.clear();
	}

	//handle incoming connection on listener
	void establish_incoming()
	{
		int new_FD = 0;
		while(new_FD != -1){
			new_FD = network_wrapper::accept_socket(listener);
			if(new_FD != -1){
				#ifndef WIN32
				if(new_FD >= FD_SETSIZE){
					LOGGER << "exceeded FD_SETSIZE, disconnect socket " << new_FD;
					close(new_FD);
					continue;
				}
				#endif
				LOGGER << "IP " << network_wrapper::get_IP(new_FD) << " port "
					<< network_wrapper::get_port(new_FD) << " sock " << new_FD;
				add_socket(new_FD);
				queue_job(new_FD);
			}
		}
	}

	//close sockets finish_job wants disconnected
	void process_disconnect()
	{
		boost::mutex::scoped_lock lock(disconnect_mutex);
		for(int x=0; x<disconnect.size(); ++x){
			remove_socket(disconnect[x], false);
			network_wrapper::disconnect(disconnect[x]);
		}
		disconnect.clear();
	}

	//monitor sockets in progress of connecting
	void process_connecting()
	{
		boost::mutex::scoped_lock lock(connecting_mutex);
		for(int x=0; x<connecting.size(); ++x){
			add_socket(connecting[x]->socket_FD, false);
			Socket_Data.insert(std::make_pair(connecting[x]->socket_FD, connecting[x]));
			FD_SET(connecting[x]->socket_FD, &master_write_FDS);
			FD_SET(connecting[x]->socket_FD, &connect_FDS);
		}
		connecting.clear();
	}

	//monitor socket that just connected, or that worker just finished with
	void process_job_finished()
	{
		boost::mutex::scoped_lock lock(job_finished_mutex);
		for(int x=0; x<job_finished.size(); ++x){
			add_socket(job_finished[x]->socket_FD, false);
			Socket_Data.insert(std::make_pair(job_finished[x]->socket_FD, job_finished[x]));
			if(!job_finished[x]->send_buff.empty()){
				//send buff contains data, check socket for writeability
				FD_SET(job_finished[x]->socket_FD, &master_write_FDS);
			}
		}
		job_finished.clear();
	}

	//schedule socket for worker to look at
	void queue_job(const int socket_FD)
	{
		boost::mutex::scoped_lock lock(job_mutex);
		std::map<int, boost::shared_ptr<socket_data> >::iterator iter = Socket_Data.find(socket_FD);
		assert(iter != Socket_Data.end());
		job.push_back(iter->second);
		remove_socket(iter->first);
		job_cond.notify_one();
	}

	//remove socket from all fd_set's, adjust min_FD and end_FD.
	void remove_socket(const int socket_FD, bool remove_socket_data = true)
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

		if(remove_socket_data){
			std::map<int, boost::shared_ptr<socket_data> >::iterator iter = Socket_Data.find(socket_FD);
			assert(iter != Socket_Data.end());
			Socket_Data.erase(socket_FD);
		}
	}

	//main network loop
	void select_loop()
	{
		int n_bytes;          //how many bytes sent/received on send() recv()
		timeval tv;           //select() timeout
		bool send_buff_empty; //used for pre/post send buff comparison
		bool socket_serviced; //keeps track of if socket was serviced on for loop socket check iteration
		bool need_call_back;  //true when socket needs to have recv_call_back done
		int service;          //set by select to how many sockets need to be serviced
		std::map<int, boost::shared_ptr<socket_data> >::iterator SD_iter;
		std::time_t Time = std::time(NULL);

		//rate limiting related
		int max_upload;              //max bytes that can be sent to stay under max upload rate
		int max_download;            //max bytes that can be received to stay under max download rate
		int max_upload_per_socket;   //max bytes that can be received to maintain rate limit
		int max_download_per_socket; //max bytes that can be sent to maintain rate limit
		int temp;

		//main loop
		while(true){
			boost::this_thread::interruption_point();

			if(Time != std::time(NULL)){
				Time = std::time(NULL);
				check_timeouts();
			}

			process_disconnect();
			process_connecting();
			process_job_finished();

			max_upload = Rate_Limit.upload_rate_control();
			max_download = Rate_Limit.download_rate_control();

			read_FDS = master_read_FDS;
			if(max_upload != 0){
				//check for writeability of all sockets
				write_FDS = master_write_FDS;
			}else{
				//rate limit reached, only check for new connections
				write_FDS = connect_FDS;
			}

			/*
			These must be initialized every iteration on linux(and possibly other OS's)
			because linux will change them to reflect the time that select() has
			blocked(POSIX.1-2001 allows this) for.
			*/
			tv.tv_sec = 0; tv.tv_usec = 1000000 / 1; // 1/100 second

			service = select(end_FD, &read_FDS, &write_FDS, NULL, &tv);

			if(service == -1){
				#ifndef WIN32
				if(errno != EINTR){
					network_wrapper::error();
				}
				#endif
			}else if(service != 0){
				/*
				There is a starvation problem that can happen when hitting the rate
				limit where the lower sockets hog available bandwidth and the higher
				sockets get starved.

				Because of this we set the maximum send such that in the worst case
				each socket (that needs to be serviced) gets to send/recv and equal
				amount of data.
				*/
				max_upload_per_socket = max_upload / service;
				max_download_per_socket = max_download / service;
				if(max_upload_per_socket == 0){
					max_upload_per_socket = 1;
				}
				if(max_download_per_socket == 0){
					max_download_per_socket = 1;
				}

				for(int x=begin_FD; x < end_FD && service; ++x){
					socket_serviced = false;
					need_call_back = false;

					//discard any selfpipe bytes
					if(x == selfpipe_read){
						if(FD_ISSET(x, &read_FDS)){
							recv(x, selfpipe_discard, sizeof(selfpipe_discard), MSG_NOSIGNAL);
							socket_serviced = true;
						}
						FD_CLR(x, &read_FDS);
						FD_CLR(x, &write_FDS);
					}

					//handle incoming connections
					if(x == listener){
						if(FD_ISSET(x, &read_FDS)){
							establish_incoming();
							socket_serviced = true;
						}
						FD_CLR(x, &read_FDS);
						FD_CLR(x, &write_FDS);
					}

					if(FD_ISSET(x, &read_FDS)){
						SD_iter = Socket_Data.find(x);
						assert(SD_iter != Socket_Data.end());

						/*
						If the rate limit is such that there is a max_download less
						than the number of sockets to service there is a case where
						not every socket will get to be serviced.
						*/
						if(max_download_per_socket > max_download){
							temp = max_download;
						}else{
							temp = max_download_per_socket;
						}

						//make sure we don't exceed maximum recv size
						if(temp > 4096){
							temp = 4096;
						}

						if(temp != 0){
							//reserve space for incoming bytes at end of buffer
							SD_iter->second->recv_buff.tail_reserve(temp);

							/*
							Some implementations might return -1 to indicate an error
							even though select() said the socket was readable. We ignore
							that when that happens.
							*/
							n_bytes = recv(x, (char *)SD_iter->second->recv_buff.tail_start(), temp, MSG_NOSIGNAL);
							if(n_bytes == 0){
								SD_iter->second->disconnect_flag = true;
								need_call_back = true;
							}else if(n_bytes > 0){
								SD_iter->second->recv_buff.tail_resize(n_bytes);
								SD_iter->second->last_seen = std::time(NULL);
								Rate_Limit.add_download_bytes(n_bytes);
								SD_iter->second->recv_flag = true;
								need_call_back = true;
							}
						}
						socket_serviced = true;
					}

					if(FD_ISSET(x, &write_FDS)){
						if(FD_ISSET(x, &connect_FDS)){
							//outgoing async connection attempt succeeded
							FD_CLR(x, &connect_FDS);
							need_call_back = true;
						}else{
							//need to send data
							SD_iter = Socket_Data.find(x);
							assert(SD_iter != Socket_Data.end());
							if(!SD_iter->second->send_buff.empty()){

								/*
								If the rate limit is such that there is a max_upload less
								than the number of sockets to service there is a case where
								not every socket will get to be serviced.
								*/
								if(max_upload_per_socket > max_upload){
									temp = max_upload;
								}else{
									temp = max_upload_per_socket;
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
									n_bytes = send(x, (char *)SD_iter->second->send_buff.data(),
										SD_iter->second->send_buff.size(), MSG_NOSIGNAL);
									if(n_bytes == 0){
										SD_iter->second->disconnect_flag = true;
										need_call_back = true;
									}else if(n_bytes > 0){
										SD_iter->second->last_seen = std::time(NULL);
										send_buff_empty = SD_iter->second->send_buff.empty();
										SD_iter->second->send_buff.erase(0, n_bytes);
										Rate_Limit.add_upload_bytes(n_bytes);
										if(!send_buff_empty && SD_iter->second->send_buff.empty()){
											//send_buff went from non-empty to empty
											SD_iter->second->send_flag = true;
											need_call_back = true;
										}
									}
								}
							}
						}
						socket_serviced = true;
					}

					/*
					Select returns the number of sockets that need to be serviced. By
					keeping track of how many we've seen we might be able to stop
					checking early.
					*/
					if(socket_serviced){
						--service;
					}

					if(need_call_back){
						queue_job(x);
					}
				}
			}
		}
	}

	/*
	Accept incoming connections on specified port. Called by ctor if listening
	port is specified as last ctor parameter.
	*/
	void start_listener(const std::string & port)
	{
		assert(listener == -1);

		network_wrapper::info Info = network_wrapper::get_info(NULL, port.c_str());
		assert(Info.resolved());
		listener = network_wrapper::create_socket(Info);
		network_wrapper::reuse_port(listener);

		//set listener to non-blocking, incoming connections inherit this
		network_wrapper::set_non_blocking(listener);

		//bind to port
		if(!network_wrapper::bind_socket(listener, Info)){
			network_wrapper::error();
			exit(1);
		}

		//start listener, set max incoming connection backlog
		if(listen(listener, 64) == -1){
			network_wrapper::error();
		}

		LOGGER << "listen port " << port;
		add_socket(listener, false);
	}

	void socket_pair(int & selfpipe_read, int & selfpipe_write)
	{
		//find an available socket by trying to bind to different ports
		int tmp_listener;
		std::string port;
		for(int x=1024; x<65536; ++x){
			std::stringstream ss;
			ss << x;
			network_wrapper::info Info = network_wrapper::get_info("127.0.0.1", ss.str().c_str());
			assert(Info.resolved());
			tmp_listener = network_wrapper::create_socket(Info);
			network_wrapper::reuse_port(tmp_listener);
			if(network_wrapper::bind_socket(tmp_listener, Info)){
				port = ss.str();
				break;
			}
		}

		if(listen(tmp_listener, 1) == -1){
			network_wrapper::error();
		}

		//connect socket for writer end of the pair
		network_wrapper::info Info = network_wrapper::get_info("127.0.0.1", port.c_str());
		assert(Info.resolved());
		selfpipe_write = network_wrapper::create_socket(Info);
		if(!network_wrapper::connect_socket(selfpipe_write, Info)){
			network_wrapper::error();
			exit(1);
		}

		//accept socket for reader end of the pair
		if((selfpipe_read = network_wrapper::accept_socket(tmp_listener)) == -1){
			network_wrapper::error();
			exit(1);
		}

		network_wrapper::disconnect(tmp_listener);
	}

	//starts self pipe used to force select to return
	void start_selfpipe()
	{
		socket_pair(selfpipe_read, selfpipe_write);
		LOGGER << "selfpipe read " << selfpipe_read << " write " << selfpipe_write;
		network_wrapper::set_non_blocking(selfpipe_read);
		network_wrapper::set_non_blocking(selfpipe_write);
		add_socket(selfpipe_read, false);
	}
};
#endif
