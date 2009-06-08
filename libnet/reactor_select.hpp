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
		boost::function<void (socket_data::socket_data_visible &)> failed_connect_call_back_in,
		boost::function<void (socket_data::socket_data_visible &)> connect_call_back_in,
		boost::function<void (socket_data::socket_data_visible &)> disconnect_call_back_in
	):
		network::reactor(
			failed_connect_call_back_in,
			connect_call_back_in,
			disconnect_call_back_in
		),
		begin_FD(0),
		end_FD(1),
		listener(-1)
	{
		//initialize fd_set
		FD_ZERO(&master_read_FDS);
		FD_ZERO(&master_write_FDS);
		FD_ZERO(&read_FDS);
		FD_ZERO(&write_FDS);

		//start self pipe used to get select to return
		start_selfpipe();

		main_loop_thread = boost::thread(boost::bind(&reactor_select::main_loop, this));
	}

	virtual ~reactor_select()
	{
		main_loop_thread.interrupt();
		main_loop_thread.join();
		wrapper::disconnect(listener);
		wrapper::disconnect(selfpipe_read);
		wrapper::disconnect(selfpipe_write);
	}

	virtual void connect(const std::string & host, const std::string & port)
	{
//DNS resolution should not be done in caller thread
		boost::shared_ptr<wrapper::info> Info = wrapper::get_info(host.c_str(), port.c_str());
		if(!Info->resolved()){
			LOGGER << "failed to resolve host " << host << " port " << port;
			return;
		}

		int new_FD = wrapper::create_socket(Info);

		//non-blocking required for async connect
		wrapper::set_non_blocking(new_FD);

		if(wrapper::connect_socket(new_FD, Info)){
			//socket connected right away, rare but it might happen
			boost::mutex::scoped_lock lock(job_finished_mutex);
			job_finished.push_back(boost::shared_ptr<socket_data>(
				new socket_data(new_FD, host, wrapper::get_IP(Info), port, Info)));
		}else{
			//socket in progress of connecting
			boost::mutex::scoped_lock lock(connecting_mutex);
			connecting.push_back(boost::shared_ptr<socket_data>(
				new socket_data(new_FD, host, wrapper::get_IP(Info), port, Info)));
		}
		send(selfpipe_write, "0", 1, MSG_NOSIGNAL);
	}

	virtual boost::shared_ptr<socket_data> get_job()
	{
		boost::mutex::scoped_lock lock(job_mutex);
		if(!job.empty()){
			boost::shared_ptr<socket_data> info = job.front();
			job.pop_front();
			return info;
		}else{
			return boost::shared_ptr<socket_data>();
		}
	}

	virtual void finish_job(boost::shared_ptr<socket_data> & info)
	{
		boost::mutex::scoped_lock lock(job_finished_mutex);
		if(info->disconnect_flag){
			boost::mutex::scoped_lock lock(disconnect_pending_mutex);
			disconnect_pending.push_back(info->socket_FD);
		}else if(!info->failed_connect_flag){
			job_finished.push_back(info);
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
	int listener; //socket of listener (-1 means no listener)

/*
int listener_IPv4;
int listener_IPv6;
*/
	/*
	This is used for the selfpipe trick. A pipe is opened and put in the fd_set
	select() will monitor. We can asynchronously signal select() to return by
	writing a byte to the read FD of the pipe.
	*/
	int selfpipe_read;  //put in master_FDS
	int selfpipe_write; //written to get select to return
	char selfpipe_discard[1024]; //used to discard selfpipe bytes

	//connected socket associated with socket_data
	std::map<int, boost::shared_ptr<socket_data> > Socket_Data;

	/*
	When a socket needs to have a call back done it is queued in this container
	along with the socket_data.

	The socket_data doesn't need to be locked because it's guaranteed that the
	select_loop_thread won't touch it.
	*/
	std::deque<boost::shared_ptr<socket_data> > job;
	boost::mutex job_mutex;

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
	boost::mutex disconnect_pending_mutex;
	std::vector<int> disconnect_pending;

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
			Socket_Data.insert(std::make_pair(socket_FD, new socket_data(socket_FD, "",
				wrapper::get_IP(socket_FD), wrapper::get_port(socket_FD))));
		}
	}

	void check_timeouts()
	{
		//temp vector needed because queue_job invalidates iterator
		std::vector<int> job_tmp;

		std::map<int, boost::shared_ptr<socket_data> >::iterator
		iter_cur = Socket_Data.begin(),
		iter_end = Socket_Data.end();
		while(iter_cur != iter_end){
			if(std::time(NULL) - iter_cur->second->last_seen > 4){
				if(FD_ISSET(iter_cur->first, &connect_FDS)){
					iter_cur->second->failed_connect_flag = true;
					job_tmp.push_back(iter_cur->first);
				}else{
					iter_cur->second->disconnect_flag = true;
					job_tmp.push_back(iter_cur->first);
				}
			}
			++iter_cur;
		}

		for(int x=0; x<job_tmp.size(); ++x){
			queue_job(job_tmp[x]);
		}

		job_tmp.clear();
	}

	//handle incoming connection on listener
	void establish_incoming()
	{
		int new_FD = 0;
		while(new_FD != -1){
			new_FD = wrapper::accept_socket(listener);
			if(new_FD != -1){
				#ifndef WIN32
				if(new_FD >= FD_SETSIZE){
					LOGGER << "exceeded FD_SETSIZE, disconnect socket " << new_FD;
					wrapper::disconnect(new_FD);
					continue;
				}
				#endif
				LOGGER << "IP " << wrapper::get_IP(new_FD) << " port "
					<< wrapper::get_port(new_FD) << " sock " << new_FD;
				add_socket(new_FD);
				queue_job(new_FD);
			}
		}
	}

	//close sockets finish_job wants disconnected
	void process_disconnect()
	{
		boost::mutex::scoped_lock lock(disconnect_pending_mutex);
		for(int x=0; x<disconnect_pending.size(); ++x){
			remove_socket(disconnect_pending[x], false);
			wrapper::disconnect(disconnect_pending[x]);
		}
		disconnect_pending.clear();
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
		worker_cond.notify_one();
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
	void main_loop()
	{
		std::time_t Time = std::time(NULL);
		while(true){
			boost::this_thread::interruption_point();
			if(Time != std::time(NULL)){
				Time = std::time(NULL);
				check_timeouts();
			}
			process_disconnect();
			process_connecting();
			process_job_finished();

			/*
			Maximum bytes that can be send()'d/recv()'d before exceeding maximum
			upload/download rate. These are decremented by however many bytes are
			send()'d/recv()'d.
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

			timeval tv;
			tv.tv_sec = 0; tv.tv_usec = 1000000 / 100; //1/100 sec timeout
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

				bool socket_serviced;
				bool need_call_back;
				for(int x=begin_FD; x < end_FD && service; ++x){
					socket_serviced = false;
					need_call_back = false;
					if(x == selfpipe_read){
						main_loop_discard_selfpipe(x, socket_serviced);
					}
					if(x == listener){
						main_loop_establish_incoming(x, socket_serviced);
					}
					if(FD_ISSET(x, &write_FDS) && FD_ISSET(x, &connect_FDS)){
						need_call_back = true;
						FD_CLR(x, &write_FDS);
						FD_CLR(x, &connect_FDS);
					}
					if(FD_ISSET(x, &read_FDS)){
						main_loop_recv(x, max_recv_per_socket, max_recv, socket_serviced, need_call_back);
					}
					if(FD_ISSET(x, &write_FDS)){
						main_loop_send(x, max_send_per_socket, max_send, socket_serviced, need_call_back);
					}
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

	//discard incoming selfpipe bytes
	void main_loop_discard_selfpipe(const int socket_FD, bool & socket_serviced)
	{
		if(FD_ISSET(socket_FD, &read_FDS)){
			recv(socket_FD, selfpipe_discard, sizeof(selfpipe_discard), MSG_NOSIGNAL);
			socket_serviced = true;
		}
		FD_CLR(socket_FD, &read_FDS);
		FD_CLR(socket_FD, &write_FDS);
	}

	//handle incoming connections
	void main_loop_establish_incoming(const int socket_FD, bool & socket_serviced)
	{
		if(FD_ISSET(socket_FD, &read_FDS)){
			establish_incoming();
			socket_serviced = true;
		}
		FD_CLR(socket_FD, &read_FDS);
		FD_CLR(socket_FD, &write_FDS);
	}

	//recv data from a socket
	void main_loop_recv(const int socket_FD, const int max_recv_per_socket,
		int & max_recv, bool & socket_serviced, bool & need_call_back)
	{
		std::map<int, boost::shared_ptr<socket_data> >::iterator SD_iter = Socket_Data.find(socket_FD);
		assert(SD_iter != Socket_Data.end());

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
			SD_iter->second->recv_buff.tail_reserve(temp);

			/*
			Some implementations might return -1 to indicate an error
			even though select() said the socket was readable. We ignore
			that when that happens.
			*/
			int n_bytes = recv(socket_FD, (char *)SD_iter->second->recv_buff.tail_start(), temp, MSG_NOSIGNAL);
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

	//send data to a socket
	void main_loop_send(const int socket_FD, const int max_send_per_socket,
		int & max_send, bool & socket_serviced, bool & need_call_back)
	{
		std::map<int, boost::shared_ptr<socket_data> >::iterator SD_iter = Socket_Data.find(socket_FD);
		assert(SD_iter != Socket_Data.end());
		if(!SD_iter->second->send_buff.empty()){
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
				int n_bytes = send(socket_FD, (char *)SD_iter->second->send_buff.data(),
					SD_iter->second->send_buff.size(), MSG_NOSIGNAL);
				if(n_bytes == 0){
					SD_iter->second->disconnect_flag = true;
					need_call_back = true;
				}else if(n_bytes > 0){
					SD_iter->second->last_seen = std::time(NULL);
					bool send_buff_empty = SD_iter->second->send_buff.empty();
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
		socket_serviced = true;
	}

	/*
	Accept incoming connections on specified port. Called by ctor if listening
	port is specified as last ctor parameter.
	*/
//DEBUG, move this to wrapper
	void start_listener(const std::string & port)
	{
		assert(listener == -1);

		boost::shared_ptr<wrapper::info> Info = wrapper::get_info(NULL, port.c_str());
		assert(Info->resolved());
		listener = wrapper::create_socket(Info);
		wrapper::reuse_port(listener);

		//set listener to non-blocking, incoming connections inherit this
		wrapper::set_non_blocking(listener);

		//bind to port
		if(!wrapper::bind_socket(listener, Info)){
			wrapper::error();
		}

		//start listener, set max incoming connection backlog
		if(listen(listener, 64) == -1){
			wrapper::error();
		}

		LOGGER << "listen port " << port;
		add_socket(listener, false);
	}

	//starts self pipe used to force select to return
	void start_selfpipe()
	{
		wrapper::socket_pair(selfpipe_read, selfpipe_write);
		LOGGER << "selfpipe read " << selfpipe_read << " write " << selfpipe_write;
		wrapper::set_non_blocking(selfpipe_read);
		wrapper::set_non_blocking(selfpipe_write);
		add_socket(selfpipe_read, false);
	}
};
}//end of network namespace
#endif
