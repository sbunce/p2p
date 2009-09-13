#include "reactor_select.hpp"

network::reactor_select::reactor_select(
	const bool duplicates_allowed_in,
	const std::string & port
):
	duplicates_allowed(duplicates_allowed_in),
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
	if(port != ""){
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
	wrapper::set_non_blocking(selfpipe_read);
	wrapper::set_non_blocking(selfpipe_write);
	add_socket(selfpipe_read);

	//set default maximum connections
	max_connections(supported_connections() / 2, supported_connections() / 2);
}

network::reactor_select::~reactor_select()
{
	close(listener_IPv4);
	close(listener_IPv6);
	close(selfpipe_read);
	close(selfpipe_write);
}

void network::reactor_select::add_socket(const int socket_FD)
{
	assert(socket_FD >= 0);
	FD_SET(socket_FD, &master_read_FDS);
	if(socket_FD < begin_FD){
		begin_FD = socket_FD;
	}else if(socket_FD >= end_FD){
		end_FD = socket_FD + 1;
	}
}

void network::reactor_select::add_socket(boost::shared_ptr<sock> & S)
{
	add_socket(S->socket_FD);
	Monitored.insert(std::make_pair(S->socket_FD, S));
	if(!S->send_buff.empty()){
		FD_SET(S->socket_FD, &master_write_FDS);
	}
}

void network::reactor_select::check_timeouts()
{
	//only check timeouts once per second since that's the granularity of our timer
	static std::time_t Time = std::time(NULL);
	if(Time == std::time(NULL)){
		return;
	}else{
		Time = std::time(NULL);
	}

	std::map<int, boost::shared_ptr<sock> >::iterator
		iter_cur = Monitored.begin(), iter_end = Monitored.end();
	while(iter_cur != iter_end){
		if(iter_cur->second->timeout()){
			boost::shared_ptr<sock> S = iter_cur->second;
			Monitored.erase(iter_cur++);
			if(FD_ISSET(S->socket_FD, &connect_FDS)){
				S->failed_connect_flag = true;
			}else{
				S->disconnect_flag = true;
			}
			S->error = sock::timed_out;
			remove_socket(S->socket_FD);
			call_back_schedule_job(S);
		}else{
			++iter_cur;
		}
	}
}

void network::reactor_select::establish_incoming(const int listener)
{
	int new_FD = 0;
	while(new_FD != -1){
		new_FD = wrapper::accept(listener);
		if(new_FD != -1){
			if(connection_incoming_limit()){
				close(new_FD);
			}else{
				boost::shared_ptr<sock> S(new sock(new_FD));
				if(!duplicates_allowed && is_duplicate(S)){
					S->failed_connect_flag = true;
					S->error = sock::duplicate;
				}else{
					connection_add(S);
				}
				call_back_schedule_job(S);
			}
		}
	}
}

boost::shared_ptr<network::sock> network::reactor_select::get_monitored(
	const int socket_FD)
{
	std::map<int, boost::shared_ptr<sock> >::iterator
		iter = Monitored.find(socket_FD);
	if(iter == Monitored.end()){
		LOGGER << "violated precondition";
		exit(1);
	}
	return iter->second;
}

void network::reactor_select::main_loop()
{
	while(true){
		boost::this_thread::interruption_point();

		process_connect_job();
		process_finished_job();
		check_timeouts();

		/*
		Maximum bytes that can be send()'d/recv()'d before exceeding maximum
		upload/download rate. These are decremented by however many bytes are
		send()'d/recv()'d during the socket scan loop.
		*/
		int max_send = Rate_Limit.available_upload();
		int max_recv = Rate_Limit.available_download();

		read_FDS = master_read_FDS;
		timeval tv;
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
			tv.tv_sec = 0; tv.tv_usec = 1000000 / 20;
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
			This loop discards selfpipe bytes, handles incoming connections, and
			determines what sockets need read/write. We don't service sockets
			for read/write at this point because we don't know how many there
			are.
			*/
			std::vector<int> read_socket, write_socket;
			for(int socket_FD = begin_FD; socket_FD < end_FD && service; ++socket_FD){
				if(socket_FD == selfpipe_read){
					if(FD_ISSET(socket_FD, &read_FDS)){
						--service;
						static char selfpipe_discard[512];
						recv(socket_FD, selfpipe_discard, sizeof(selfpipe_discard), MSG_NOSIGNAL);
					}
				}else if(socket_FD == listener_IPv4){
					if(FD_ISSET(socket_FD, &read_FDS)){
						--service;
						establish_incoming(socket_FD);
					}
				}else if(socket_FD == listener_IPv6){
					if(FD_ISSET(socket_FD, &read_FDS)){
						--service;
						establish_incoming(socket_FD);
					}
				}else if(FD_ISSET(socket_FD, &write_FDS) && FD_ISSET(socket_FD, &connect_FDS)){
					--service;
					if(wrapper::async_connect_succeeded(socket_FD)){
						boost::shared_ptr<sock> S = get_monitored(socket_FD);
						remove_socket(S);
						call_back_schedule_job(S);
					}else{
						//see if there is another address to try
						boost::shared_ptr<sock> S = get_monitored(socket_FD);
						remove_socket(S);
						S->info->next();
						if(S->info->resolved()){
							//another address to try
							reactor::schedule_connect(S);
						}else{
							//no more addresses to try, connect failed
							S->failed_connect_flag = true;
							call_back_schedule_job(S);
						}
					}
				}else{
					if(FD_ISSET(socket_FD, &read_FDS)){
						read_socket.push_back(socket_FD);
					}
					if(FD_ISSET(socket_FD, &write_FDS)){
						write_socket.push_back(socket_FD);
					}
					if(FD_ISSET(socket_FD, &read_FDS) || FD_ISSET(socket_FD, &write_FDS)){
						--service;
					}
				}
			}

			/*
			We use a set because one socket might both read and write. If it
			does we only need to schedule one job.
			*/
			std::set<int> job;

			//try to read sockets that were in read_FDS
			if(!read_socket.empty()){
				int max_recv_per_socket = max_recv / read_socket.size();
				for(std::vector<int>::iterator iter_cur = read_socket.begin(),
					iter_end = read_socket.end(); iter_cur != iter_end; ++iter_cur)
				{
					bool schedule_job = false;
					main_loop_recv(*iter_cur, max_recv_per_socket, max_recv, schedule_job);
					if(schedule_job){
						job.insert(*iter_cur);
					}
				}
			}

			//try to write sockets that were in write_FDS
			if(!write_socket.empty()){
				int max_send_per_socket = max_send / write_socket.size();
				for(std::vector<int>::iterator iter_cur = write_socket.begin(),
					iter_end = write_socket.end(); iter_cur != iter_end; ++iter_cur)
				{
					bool schedule_job = false;
					main_loop_send(*iter_cur, max_send_per_socket, max_send, schedule_job);
					if(schedule_job){
						job.insert(*iter_cur);
					}
				}
			}

			//schedule jobs for sockets that either did read, write, or both
			for(std::set<int>::iterator iter_cur = job.begin(),
				iter_end = job.end(); iter_cur != iter_end; ++iter_cur)
			{
				boost::shared_ptr<sock> S = get_monitored(*iter_cur);
				remove_socket(S);
				call_back_schedule_job(S);
			}
		}
	}
}

void network::reactor_select::main_loop_recv(const int socket_FD,
	const int max_recv_per_socket, int & max_recv, bool & schedule_job)
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
	if(temp > MTU){
		temp = MTU;
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
		}else if(n_bytes > 0){
			S->recv_buff.tail_resize(n_bytes);
			S->seen();
			Rate_Limit.add_download_bytes(n_bytes);
			max_recv -= n_bytes;
			S->latest_recv = n_bytes;
			S->recv_flag = true;
		}
		schedule_job = true;
	}
}

void network::reactor_select::main_loop_send(const int socket_FD,
	const int max_send_per_socket, int & max_send, bool & schedule_job)
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
		if(temp > MTU){
			temp = MTU;
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
				S->latest_send = n_bytes;
				if(!send_buff_empty && S->send_buff.empty()){
					//send_buff went from non-empty to empty
					S->send_flag = true;
					schedule_job = true;
				}
			}
		}
	}
}

void network::reactor_select::process_connect_job()
{
	boost::shared_ptr<sock> S;
	while(S = connect_job_get()){
		if(!duplicates_allowed && is_duplicate(S)){
			S->failed_connect_flag = true;
			S->error = sock::duplicate;
			call_back_schedule_job(S);
			continue;
		}

		//check soft connection limit
		if(connection_outgoing_limit()){
			S->failed_connect_flag = true;
			S->error = sock::max_connections;
			call_back_schedule_job(S);
			return;
		}

		//check hard connection limit
		#ifdef _WIN32
		if(master_read_FDS.fd_count >= FD_SETSIZE){
		#else
		if(S->socket_FD >= FD_SETSIZE){
		#endif
			S->failed_connect_flag = true;
			S->error = sock::max_connections;
			call_back_schedule_job(S);
			return;
		}

		assert(S->socket_FD == -1);
		const_cast<int &>(S->socket_FD) = wrapper::socket(*S->info);

		if(S->socket_FD == -1){
			S->failed_connect_flag = true;
			S->error = sock::other_error;
			call_back_schedule_job(S);
			return;
		}

		wrapper::set_non_blocking(S->socket_FD);
		connection_add(S);
		if(wrapper::connect(S->socket_FD, *S->info)){
			//socket connected right away, rare but it might happen
			call_back_schedule_job(S);
		}else{
			//socket in progress of connecting
			add_socket(S);
			FD_SET(S->socket_FD, &master_write_FDS);
			FD_SET(S->socket_FD, &connect_FDS);
		}
	}
}

void network::reactor_select::process_finished_job()
{
	while(boost::shared_ptr<sock> S = call_back_finished_job()){
		add_socket(S);
	}
}

void network::reactor_select::process_force()
{
	std::map<int, boost::shared_ptr<sock> >::iterator
		iter_cur = Monitored.begin(),
		iter_end = Monitored.end();
	while(iter_cur != iter_end && force_pending()){
		//only force a call back if the sock is connected, and it needs to be forced
		if(iter_cur->second->connect_flag && force_check(iter_cur->second)){
			iter_cur->second->recv_flag = true;
			iter_cur->second->latest_recv = 0;
			call_back_schedule_job(iter_cur->second);
			Monitored.erase(iter_cur++);
		}else{
			++iter_cur;
		}
	}
}

void network::reactor_select::remove_socket(const int socket_FD)
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

void network::reactor_select::remove_socket(boost::shared_ptr<sock> & S)
{
	if(Monitored.erase(S->socket_FD) != 1){
		LOGGER << "precondition violated";
		exit(1);
	}
	remove_socket(S->socket_FD);
}

void network::reactor_select::start()
{
	main_loop_thread = boost::thread(boost::bind(&reactor_select::main_loop, this));
}

void network::reactor_select::stop()
{
	main_loop_thread.interrupt();
	main_loop_thread.join();
}

unsigned network::reactor_select::supported_connections()
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

void network::reactor_select::trigger_selfpipe()
{
	send(selfpipe_write, "0", 1, MSG_NOSIGNAL);
}
