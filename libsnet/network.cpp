#include "network.hpp"

network::network(
	boost::function<void (int, buffer &, buffer &, direction)> connect_call_back_in,
	boost::function<bool (int, buffer &, buffer &)> recv_call_back_in,
	boost::function<bool (int, buffer &, buffer &)> send_call_back_in,
	boost::function<void (int)> disconnect_call_back_in,
	const int connect_time_out_in,
	const int socket_time_out_in,
	const int listen_port
):
	connect_call_back(connect_call_back_in),
	recv_call_back(recv_call_back_in),
	send_call_back(send_call_back_in),
	disconnect_call_back(disconnect_call_back_in),
	begin_FD(0),
	end_FD(1),
	listener(-1),
	writes_pending(0),
	connection_limit(500),
	outgoing_connections(0),
	incoming_connections(0),
	connect_time_out(connect_time_out_in),
	socket_time_out(socket_time_out_in)
{
	#ifdef WIN32
	WORD wsock_ver = MAKEWORD(1,1);
	WSADATA wsock_data;
	int startup;
	if((startup = WSAStartup(wsock_ver, &wsock_data)) != 0){
		LOGGER << "winsock startup error " << startup;
		exit(1);
	}
	#endif
	FD_ZERO(&master_FDS);
	FD_ZERO(&read_FDS);
	FD_ZERO(&write_FDS);
	FD_ZERO(&connect_FDS);

	if(listen_port >= 1024){
		start_listener(listen_port);
	}
	start_selfpipe();

	select_loop_thread = boost::thread(boost::bind(&network::select_loop, this));
}

network::~network()
{
	select_loop_thread.interrupt();
	select_loop_thread.join();

	//disconnect all sockets
	for(int x=begin_FD; x < end_FD; ++x){
		if(FD_ISSET(x, &master_FDS)){
			LOGGER << "disconnecting socket " << x;
			#ifdef WIN32
			closesocket(x);
			#else
			close(x);
			#endif
		}
	}

	#ifdef WIN32
	WSACleanup();
	#endif
}

bool network::add_connection(const std::string & IP, const int port)
{
	boost::recursive_mutex::scoped_lock lock(connection_queue_mutex);
	if(outgoing_connections < connection_limit){
		++outgoing_connections;
		connection_queue.push(std::make_pair(IP, port));
		write(selfpipe_write, "0", 1);
		LOGGER << "queued connection " << IP << ":" << port;
		return true;
	}else{
		LOGGER << "can't queue " << IP << ":" << port << ", connection limit reached";
		return false;
	}
}

void network::add_socket(const int socket_FD)
{
	assert(socket_FD > 0);
	FD_SET(socket_FD, &master_FDS);
	//sockets in set
	if(socket_FD < begin_FD){
		begin_FD = socket_FD;
	}else if(socket_FD >= end_FD){
		end_FD = socket_FD + 1;
	}
}

void network::check_timeouts()
{
	//check timeouts on connecting sockets
	std::map<int, boost::shared_ptr<socket_data> >::iterator
	iter_cur = Connecting_Socket_Data.begin(),
	iter_end = Connecting_Socket_Data.end();
	std::time_t current_time = std::time(NULL);
	while(iter_cur != iter_end){
		if(current_time - iter_cur->second->last_seen > connect_time_out){
			LOGGER << "timed out connection attempt to " << iter_cur->first;
			remove_socket(iter_cur->first);
			Connecting_Socket_Data.erase(iter_cur++);
		}else{
			++iter_cur;
		}
	}

	//check timeouts on connected sockets
	iter_cur = Socket_Data.begin();
	iter_end = Socket_Data.end();
	while(iter_cur != iter_end){
		if(current_time - iter_cur->second->last_seen > socket_time_out){
			remove_socket(iter_cur->first);
			disconnect_call_back(iter_cur->first);
			Socket_Data.erase(iter_cur++);
		}else{
			++iter_cur;
		}
	}
}

unsigned network::connections()
{
	return outgoing_connections + incoming_connections;
}

network::socket_data * network::create_socket_data(const int socket_FD)
{
	std::map<int, boost::shared_ptr<socket_data> >::iterator SD;
	SD = Socket_Data.find(socket_FD);
	if(SD == Socket_Data.end()){
		std::pair<std::map<int, boost::shared_ptr<socket_data> >::iterator, bool> new_SD;
		direction Direction;
		std::map<int, boost::shared_ptr<socket_data> >::iterator
			CSD = Connecting_Socket_Data.find(socket_FD);
		if(CSD == Connecting_Socket_Data.end()){
			/*
			Socket is incoming connection because it doesn't have
			Connecting_Socket_Data.
			*/
			new_SD = Socket_Data.insert(std::make_pair(socket_FD, new socket_data(writes_pending, incoming_connections)));
			assert(new_SD.second);
			Direction = INCOMING;
		}else{
			/*
			Socket is outgoing connection because it has Connecting_Socket_Data,
			move socket_data from Connecting_Socket_Data to Socket_Data to indicate
			that the socket has connected.
			*/
			FD_CLR(socket_FD, &connect_FDS);
			new_SD = Socket_Data.insert(*CSD);
			assert(new_SD.second);
			new_SD.first->second->last_seen = std::time(NULL);
			Connecting_Socket_Data.erase(socket_FD);
			Direction = OUTGOING;
		}
		bool send_buff_empty = new_SD.first->second->send_buff.empty();
		connect_call_back(socket_FD, new_SD.first->second->recv_buff, new_SD.first->second->send_buff, Direction);
		if(send_buff_empty && !new_SD.first->second->send_buff.empty()){
			//send_buff went from empty to non-empty
			++writes_pending;
		}
		return &(*new_SD.first->second);
	}else{
		return &(*SD->second);
	}
}

unsigned network::current_download_rate()
{
	return Rate_Limit.current_download_rate();
}

unsigned network::current_upload_rate()
{
	return Rate_Limit.current_upload_rate();
}

unsigned network::get_max_download_rate()
{
	return Rate_Limit.get_max_download_rate();
}

unsigned network::get_max_upload_rate()
{
	return Rate_Limit.get_max_upload_rate();
}

void network::establish_incoming_connections()
{
	int new_FD = 0;
	while(new_FD != -1){
		sockaddr_in remoteaddr;
		socklen_t len = sizeof(remoteaddr);
		new_FD = accept(listener, (sockaddr *)&remoteaddr, &len);
		if(new_FD != -1){
			//connection established
			std::string new_IP(inet_ntoa(remoteaddr.sin_addr));
			LOGGER << new_IP << " socket " << new_FD << " connected";
			if(incoming_connections < connection_limit){
				add_socket(new_FD);
				create_socket_data(new_FD);
				++incoming_connections;
			}else{
				LOGGER << "incoming_connection limit exceeded, disconnecting " << new_IP;
				break;
			}
		}
	}
}

void network::establish_outgoing_connection(const std::string & IP, const int port)
{
	sockaddr_in dest;
	int new_FD = socket(PF_INET, SOCK_STREAM, 0);

	/*
	Set socket to be non-blocking. Trying to read a O_NONBLOCK socket will result
	in -1 being returned and errno would be set to EWOULDBLOCK.
	*/
	fcntl(new_FD, F_SETFL, O_NONBLOCK);

	dest.sin_family = AF_INET;   //IPV4
	dest.sin_port = htons(port); //port to connect to

	/*
	The inet_aton function is not used because winsock doesn't have it. Instead
	the inet_addr function is used which is 'almost' equivalent.

	If you try to connect to 255.255.255.255 the return value is -1 which is the
	correct conversion, but also the same return as an error. This is ambiguous
	so a special case is needed for it.
	*/
	if(IP == "255.255.255.255"){
		dest.sin_addr.s_addr = inet_addr(IP.c_str());
	}else if((dest.sin_addr.s_addr = inet_addr(IP.c_str())) == -1){
		LOGGER << "invalid IP " << IP;
		return;
	}

	/*
	The sockaddr_in struct is smaller than the sockaddr struct. The sin_zero
	member of sockaddr_in is padding to make them the same size. It is required
	that the sine_zero space be zero'd out. Not doing so can result in undefined
	behavior.
	*/
	std::memset(&dest.sin_zero, 0, sizeof(dest.sin_zero));

	if(connect(new_FD, (sockaddr *)&dest, sizeof(dest)) == -1){
		/*
		Socket is in progress of connecting. Add to connect_FDS. It will be ready
		to read/write once the socket shows up as writeable.
		*/
		LOGGER << "async connection " << IP << ":" << port << " socket " << new_FD;
		FD_SET(new_FD, &connect_FDS);
	}else{
		//socket connected right away, rare but it might happen
		LOGGER << "connection " << IP << ":" << port << " socket " << new_FD;
	}

	Connecting_Socket_Data.insert(std::make_pair(new_FD, new socket_data(writes_pending, outgoing_connections)));
	add_socket(new_FD);
}

void network::process_connection_queue()
{
	//establish connections queued by add_connection()
	std::pair<std::string, int> new_connect_info;
	while(true){
		{//begin lock scope
		boost::recursive_mutex::scoped_lock lock(connection_queue_mutex);
		if(connection_queue.empty()){
			break;
		}else{
			new_connect_info = connection_queue.front();
			connection_queue.pop();
		}
		}//end lock scope
		establish_outgoing_connection(new_connect_info.first, new_connect_info.second);
	}
}

void network::remove_socket(const int socket_FD)
{
	#ifdef WIN32
	closesocket(socket_FD);
	#else
	close(socket_FD);
	#endif
	FD_CLR(socket_FD, &master_FDS);
	FD_CLR(socket_FD, &read_FDS);
	FD_CLR(socket_FD, &write_FDS);
	FD_CLR(socket_FD, &connect_FDS);

	//update end_FD
	for(int x=end_FD; x>begin_FD; --x){
		if(FD_ISSET(x, &master_FDS)){
			end_FD = x + 1;
			break;
		}
	}
	//update begin_FD
	for(int x=begin_FD; x<end_FD; ++x){
		if(FD_ISSET(x, &master_FDS)){
			begin_FD = x;
			break;
		}
	}
}

void network::start_listener(const int port)
{
	//assert not already listening on socket
	assert(listener == -1);

	//set listening socket
	if((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		#ifdef WIN32
		LOGGER << "winsock error " << WSAGetLastError();
		#else
		perror("socket");
		#endif
		exit(1);
	}

	//reuse port if it's currently in use
	int yes = 1;
	if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
		#ifdef WIN32
		LOGGER << "winsock error " << WSAGetLastError();
		#else
		perror("setsockopt");
		#endif
		exit(1);
	}

	/*
	Listener set to non-blocking. Also all incoming connections will inherit this.
	*/
	fcntl(listener, F_SETFL, O_NONBLOCK);

	//prepare listener info
	sockaddr_in myaddr;                  //server address
	myaddr.sin_family = AF_INET;         //ipv4
	myaddr.sin_addr.s_addr = INADDR_ANY; //listen on all interfaces
	myaddr.sin_port = htons(port);       //listening port

	//docs for this in establish_connection function
	std::memset(&myaddr.sin_zero, 0, sizeof(myaddr.sin_zero));

	//set listener info
	if(bind(listener, (sockaddr *)&myaddr, sizeof(myaddr)) == -1){
		#ifdef WIN32
		LOGGER << "winsock error " << WSAGetLastError();
		#else
		perror("bind");
		#endif
		exit(1);
	}

	//start listener socket listening
	const int backlog = 64; //how many pending connections can exist
	if(listen(listener, backlog) == -1){
		#ifdef WIN32
		LOGGER << WSAGetLastError();
		#else
		perror("listen");
		#endif
		exit(1);
	}
	LOGGER << "listening socket " << listener << " port " << port;
	add_socket(listener);
}

void network::select_loop()
{
	char recv_buff[16384];       //main recv buffer (size of largest ethernet "jumbo frame"?)
	int n_bytes;                 //how many bytes sent/received on send() recv()
	int max_upload;              //max bytes that can be sent to stay under max upload rate
	int max_download;            //max bytes that can be received to stay under max download rate
	int max_upload_per_socket;   //max bytes that can be received to maintain rate limit
	int max_download_per_socket; //max bytes that can be sent to maintain rate limit
	int temp;                    //general purpose
	timeval tv;                  //select() timeout
	bool socket_serviced;        //keeps track of if socket was serviced
	bool send_buff_empty;        //used for pre/post-call_back empty comparison
	int service;                 //set by select to how many sockets need to be serviced
	socket_data * SD;            //holder for create_socket_data return value
	std::time_t last_time(std::time(NULL)); //used to run stuff once per second

	//main loop
	while(true){
		boost::this_thread::interruption_point();

		if(std::time(NULL) != last_time){
			last_time = std::time(NULL);
			check_timeouts();
		}

		process_connection_queue();

		//timeout before select returns with no results
		tv.tv_sec = 0; tv.tv_usec = 10000; // 1/100 second

		//determine what sockets select should monitor
		max_upload = Rate_Limit.upload_rate_control();
		max_download = Rate_Limit.download_rate_control();
		if(writes_pending && max_upload != 0){
			write_FDS = master_FDS;
		}else{
			/*
			When no buffers non-empty we only check for writability on sockets in
			progress of connecting. Writeability on a socket in progress of
			connecting means the socket has connected. This will NOT have the same
			effect as checking for writeability on a connected socket which causes
			100% CPU usage when all buffers are non-empty.
			*/
			write_FDS = connect_FDS;
		}
		if(max_download != 0){
			read_FDS = master_FDS;
		}else{
			FD_ZERO(&read_FDS);
		}

		service = select(end_FD, &read_FDS, &write_FDS, NULL, &tv);

		if(service == -1){
			if(errno != EINTR){
				//fatal error
				#ifdef WIN32
				LOGGER << "winsock error " << WSAGetLastError();
				#else
				perror("select");
				#endif
				exit(1);
			}
		}else if(service != 0){
			/*
			If we were to send the maximum possible amount of data, such that
			we'd stay under the maximum rate, there is a starvation problem that
			can happen. Say we had two sockets and could send two bytes before
			reaching the maximum rate, if we always sent as much as we could,
			and the first socket always took two bytes, the second socket would
			get starved.

			Because of this we set the maximum send such that in the worst case
			each socket (that needs to be serviced) gets to send/recv and equal
			amount of data. It's possible some higher numbered sockets will get
			excluded if there are more sockets than bytes to send()/recv().
			*/
			max_upload_per_socket = max_upload / service;
			max_download_per_socket = max_download / service;
			if(max_upload_per_socket == 0){
				max_upload_per_socket = 1;
			}
			if(max_download_per_socket == 0){
				max_download_per_socket = 1;
			}
			if(max_download_per_socket > sizeof(recv_buff)){
				max_download_per_socket = sizeof(recv_buff);
			}

			for(int x=begin_FD; x < end_FD && service; ++x){
				socket_serviced = false;

				//bytes from selfpipe are discarded
				if(x == selfpipe_read){
					if(FD_ISSET(x, &read_FDS)){
						//discard recv'd bytes
						read(x, selfpipe_discard, sizeof(selfpipe_discard));
						socket_serviced = true;
					}
					if(FD_ISSET(x, &write_FDS)){
						socket_serviced = true;
					}
					if(socket_serviced){
						--service;
					}
					continue;
				}

				if(FD_ISSET(x, &read_FDS)){
					if(x == listener){
						establish_incoming_connections();
					}else{
						/*
						If there are more sockets than bytes to receive there is a case
						where even at 1 byte recv not every socket will get a turn.
						*/
						if(max_download_per_socket > max_download){
							temp = max_download;
						}else{
							temp = max_download_per_socket;
						}
						if(temp != 0){
							if((n_bytes = recv(x, recv_buff, temp, MSG_NOSIGNAL)) <= 0){
								remove_socket(x);
								Socket_Data.erase(x);
								disconnect_call_back(x);
							}else{
								max_download -= n_bytes;
								Rate_Limit.add_download_bytes(n_bytes);
								SD = create_socket_data(x);
								SD->last_seen = std::time(NULL);
								SD->recv_buff.append((unsigned char *)recv_buff, n_bytes);
								send_buff_empty = SD->send_buff.empty();
								if(!recv_call_back(x, SD->recv_buff, SD->send_buff)){
									remove_socket(x);
									Socket_Data.erase(x);
									disconnect_call_back(x);
								}else{
									if(send_buff_empty && !SD->send_buff.empty()){
										//send_buff went from empty to non-empty
										++writes_pending;
									}
								}
							}
						}
					}
					socket_serviced = true;
				}

				if(FD_ISSET(x, &write_FDS)){
					SD = create_socket_data(x);
					if(!SD->send_buff.empty()){
						/*
						If there are more sockets than bytes to send there is a case
						where even at 1 byte recv not every socket will get a turn.
						*/
						if(max_upload_per_socket > max_upload){
							temp = max_upload;
						}else{
							temp = max_upload_per_socket;
						}

						//don't try to send more than we have
						if(temp > SD->send_buff.size()){
							temp = SD->send_buff.size();
						}

						if(temp != 0){
							if((n_bytes = send(x, SD->send_buff.data(), temp, MSG_NOSIGNAL)) <= 0){
								remove_socket(x);
								Socket_Data.erase(x);
								disconnect_call_back(x);
							}else{
								max_upload -= n_bytes;
								Rate_Limit.add_upload_bytes(n_bytes);
								SD->last_seen = std::time(NULL);
								send_buff_empty = SD->send_buff.empty();
								SD->send_buff.erase(0, n_bytes);
								if(!send_buff_empty && SD->send_buff.empty()){
									//send_buff went from non-empty to empty
									--writes_pending;
								}
								send_buff_empty = SD->send_buff.empty();
								if(!send_call_back(x, SD->recv_buff, SD->send_buff)){
									remove_socket(x);
									Socket_Data.erase(x);
									disconnect_call_back(x);
								}else{
									if(send_buff_empty && !SD->send_buff.empty()){
										//send_buff went from empty to non-empty
										++writes_pending;
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
			}
		}
	}
}

void network::set_max_download_rate(const unsigned rate)
{
	return Rate_Limit.set_max_download_rate(rate);
}

void network::set_max_upload_rate(const unsigned rate)
{
	return Rate_Limit.set_max_upload_rate(rate);
}

void network::start_selfpipe()
{
	int pipe_FD[2];
	if(pipe(pipe_FD) == -1){
		LOGGER << "error creating tickle self-pipe";
		exit(1);
	}
	selfpipe_read = pipe_FD[0];
	selfpipe_write = pipe_FD[1];
	LOGGER << "created self pipe socket " << selfpipe_read;
	fcntl(selfpipe_read, F_SETFL, O_NONBLOCK);
	fcntl(selfpipe_write, F_SETFL, O_NONBLOCK);
	add_socket(selfpipe_read);
}
